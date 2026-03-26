//C:\Proyectos\Pydigitador\core\Hardware\Sensor\SensorAdapter.go
// adaptador del sensor a go
// se encarga de adaptar los metodos del sensor en go

package digitador

/*
#cgo CXXFLAGS: -std=c++11
#cgo LDFLAGS: -L${SRCDIR}/x64lib -llibzkfp
#include "SensorBridge.h"
#include <stdlib.h>
*/
import "C"

import (
	"errors"
	"fmt"
	"sync"
	"time"
	"unsafe"
)

// SensorAdapter es la estructura en Go que representa a tu Sensor.
// Encapsula el "puntero" misterioso de C (SensorHandle).

// creamos la clase sensorAdapter para poder usar los metodos del sensor en go
type SensorAdapter struct {
	handle C.SensorHandle
	mu     sync.Mutex
	// Map to convert C-Int-IDs back to Go-String-RunIDs for 1:N mode
	idToRunID map[int]string
	runIDToID map[string]int
	nextID    int
}

// sensor falso para pruebas rapidas
func SensorFalso() (*SensorAdapter, error) {
	return &SensorAdapter{handle: nil}, nil
}

// ==========================================
// 1. INICIALIZACIÓN Y CREACIÓN
// ==========================================

// creamos sensorGo para poder usar los metodos del sensor.cpp en go
func SensorGO() (*SensorAdapter, error) {
	// Llamamos a la función createSensor de C++ que nos devuelve un puntero void
	handle := C.CreateSensor()
	//se pudo crear la instancia del sensor?
	if handle == nil {
		//no, entonces devolvemos error
		return nil, errors.New("(-) [GO]:no se pudo crear la instancia del sensor en C++")
	}

	// Inicializamos el sensor
	resultado := C.InitSensor(handle)
	//se inicializo correctamente?
	if resultado == 0 {
		//no, entonces borramos la instancia del sensor
		C.DestroySensor(handle)
		return nil, errors.New("(-) [GO]:falló la inicialización del hardware del sensor")
	}

	//si, entonces devolvemos el sensor y el tipo sensorAdapter
	fmt.Println("(+)[GO]: Sensor inicializado correctamente")
	return &SensorAdapter{
		handle:    handle,
		idToRunID: make(map[int]string),
		runIDToID: make(map[string]int),
		nextID:    1,
	}, nil
}

// ==========================================
// 2. MÉTODOS DEL SENSOR (CASOS DE USO)
// ==========================================

// creamos un destructor para el sensor
func (s *SensorAdapter) Cerrar() {
	if s.handle != nil {
		C.DestroySensor(s.handle)
		s.handle = nil
		fmt.Println("(+) [Go] Sensor cerrado y memoria liberada")
	}
}

// protocolo de intentos para capturar la huella 10 intentos
func (s *SensorAdapter) CapturarHuellaTimeout(timeout time.Duration) ([]byte, error) {
	deadline := time.Now().Add(timeout)

	for time.Now().Before(deadline) {
		// CapturarHuella ya está en este mismo paquete, llama a C.AcquireFingerprint (inmediato)
		plantilla, err := s.CapturarHuella()
		if err == nil {
			return plantilla, nil
		}
		time.Sleep(100 * time.Millisecond)
	}
	return nil, errors.New("(-) [GO]: no se detectó ningún dedo o hubo un error al capturar")
}

// creamos una adaptacion de la funcion AquireFingerprint en go
func (s *SensorAdapter) CapturarHuella() ([]byte, error) {

	//para no saturar el huellero de gorutines posteriormente
	s.mu.Lock()
	defer s.mu.Unlock()

	//sensor esta inicializado?
	if s.handle == nil {
		//no, retornamos error
		return nil, errors.New("(-) [GO]:el sensor no está inicializado")
	}

	// Creamos un buffer con tamaño max de 2048 bytes para una plantilla (ajustable).
	// C++ necesita un "buffer" donde escribir los datos.
	bufferSize := 2048
	outBuffer := make([]byte, bufferSize)

	// Variable para guardar el tamaño real que devuelva C++
	var actualSize C.int = 0

	// Llamamos a C, convirtiendo nuestro bloque de Go en el tipo de dato que pide C
	resultado := C.AcquireFingerprint(
		s.handle,
		//puntero a un bloque de memoria
		(*C.uchar)(unsafe.Pointer(&outBuffer[0])),
		&actualSize,
	)

	//hubo error de hardware?
	if resultado == -1 {
		//si, retornamos error
		return nil, errors.New("(-) [GO]: hubo un error de sensor al capturar la huella")
	}

	//se detecto dedo?
	if resultado == 0 {
		//no, retornamos error
		return nil, errors.New("(-) [GO]: no se detectó ningún dedo o hubo un error al capturar")
	}

	//el tamaño es muy grande o muy pequeño?
	if int(actualSize) > bufferSize || int(actualSize) < 0 {
		//si, retornamos error
		return nil, errors.New("(-) [GO]: el tamaño de la plantilla es inválido")
	}

	// no, ajustamos el tamaño del arreglo para devolver solo los bytes válidos
	plantillaFinal := outBuffer[:int(actualSize)]
	//devolvemos la plantilla final
	return plantillaFinal, nil
}

// creamos funcion CompararHuellas para comparar plantilla1 con plantilla2
func (s *SensorAdapter) CompararHuellas(plantilla1 []byte, plantilla2 []byte) (int, error) {
	//sensor esta inicializado?
	if s.handle == nil {
		//no, retornamos error
		return 0, errors.New("(-) [GO]:el sensor no está inicializado")
	}
	//plantillas poseen datos?
	if len(plantilla1) == 0 || len(plantilla2) == 0 {
		//no, retornamos error
		return 0, errors.New("(-) [GO]: las plantillas no poseen datos")
	}
	//obtenemos los punteros de cada plantilla
	pTpl1 := (unsafe.Pointer(&plantilla1[0]))
	pTpl2 := (unsafe.Pointer(&plantilla2[0]))

	//comparamos las plantillas
	score := C.MatchTemplates(
		s.handle,
		(*C.uchar)(pTpl1),
		C.int(len(plantilla1)),
		(*C.uchar)(pTpl2),
		C.int(len(plantilla2)))

	//resultado corrupto?
	if score == -1 {
		//si, retornamos error
		return 0, errors.New("[GO]: Error al comparar las plantillas, revise las templates en la Base de datos")
	}

	//no, retornamos el score
	return int(score), nil
}

// -----------------------------------------------------------------------------
// OPERACIONES 1:N (Para modo Totem Ultra-Rápido)
// -----------------------------------------------------------------------------

// DBAdd1N agrega una huella al cache del sensor y asigna un id entero mapeado
func (s *SensorAdapter) DBAdd1N(runID string, plantilla []byte) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.handle == nil {
		return errors.New("(-) [GO]: sensor no inicializado")
	}

	// Si ya existe, nos saltamos el incremento
	id, ok := s.runIDToID[runID]
	if !ok {
		id = s.nextID
		s.nextID++
	}

	pTpl := (*C.uchar)(unsafe.Pointer(&plantilla[0]))
	res := C.DBAdd(s.handle, C.int(id), pTpl, C.int(len(plantilla)))

	if res == 0 {
		return errors.New("(-) [GO]: error al agregar huella a la cache del sensor (C++)")
	}

	// Guardamos el mapeo para poder reconocer el RUN despues
	s.idToRunID[id] = runID
	s.runIDToID[runID] = id

	return nil
}

// DBIdentify1N busca una huella en el cache y devuelve directamente el RunID
func (s *SensorAdapter) DBIdentify1N(plantilla []byte) (string, int, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	if s.handle == nil {
		return "", 0, errors.New("(-) [GO]: sensor no inicializado")
	}

	var cID, cScore C.int
	pTpl := (*C.uchar)(unsafe.Pointer(&plantilla[0]))

	res := C.DBIdentify(s.handle, pTpl, C.int(len(plantilla)), &cID, &cScore)

	if res == 0 {
		return "", 0, errors.New("no_match")
	}

	runID, ok := s.idToRunID[int(cID)]
	if !ok {
		return "", 0, errors.New("run_not_mapped")
	}

	return runID, int(cScore), nil
}
