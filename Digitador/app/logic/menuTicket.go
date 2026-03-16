// MenuTicket orquesta el flujo completo de emisión de ticket:
// 1. Captura la huella del sensor
// 2. Compara contra todos los templates en DB
// 3. Obtiene el perfil del estudiante identificado
// 4. Emite el ticket
package logic

import (
	f "fmt"
	"time"

	"Pydigitador/app/ticket"
	db "Pydigitador/core/db"
	digitador "Pydigitador/core/Hardware/Sensor"
	repo "Pydigitador/infra/DB"
)

// MenuTicket es el punto de entrada desde el menú principal para emitir un ticket.
func MenuTicket(sensor *digitador.SensorAdapter, dbRepo *repo.SQLiteUserRepository) {
	// ¿hay sensor disponible?
	if sensor == nil {
		f.Println("(!) Sensor no disponible para procesar ticket.")
		return
	}

	f.Println("Coloque su dedo en el sensor (tiene 10 segundos)...")

	// 1. Capturar huella con timeout
	plantillaCapturada, err := sensor.CapturarHuellaTimeout(10 * time.Second)
	if err != nil {
		f.Printf("(-) [GO]: No se pudo capturar la huella: %v\n", err)
		return
	}

	// 2. Comparar contra todos los templates en la base de datos
	allTemplates, err := dbRepo.ObtenerTodosTemplates()
	if err != nil {
		f.Printf("(-) [GO]: Error obteniendo templates: %v\n", err)
		return
	}

	var runIDEncontrado string
	for run, templateDB := range allTemplates {
		score, err := sensor.CompararHuellas(plantillaCapturada, templateDB)
		if err != nil {
			continue
		}
		if score >= db.MatchTreshold {
			runIDEncontrado = run
			break
		}
	}

	// 3. ¿Se encontró al estudiante?
	if runIDEncontrado == "" {
		f.Println("(-) [TICKET]: Estudiante no encontrado. Huella no registrada.")
		return
	}
	f.Printf("(+) [TICKET]: Estudiante identificado: %s\n", runIDEncontrado)

	// 4. Obtener el perfil completo del estudiante
	perfil, err := dbRepo.ObtenerPerfilPorRunID(runIDEncontrado)
	if err != nil {
		f.Printf("(-) [GO]: Error obteniendo perfil del estudiante: %v\n", err)
		return
	}

	// 5. Obtener fecha actual y tipo de ración
	fechaTXT := time.Now().Format("02/01/2006 15:04")
	fechaDB := time.Now().Format("2006-01-02")
	hora := time.Now().Hour()
	var racion string
	var tipoRacionEnum db.TipoRacion

	if hora < 11 {
		racion = "Desayuno"
		tipoRacionEnum = db.Desayuno
	} else {
		racion = "Almuerzo"
		tipoRacionEnum = db.Almuerzo
	}

	// 6. Guardar el registro en la base de datos
	nuevoRegistro := db.RegistroRacion{
		IDEstudiante:   runIDEncontrado,
		FechaServicio:  fechaDB,
		TipoRacion:     tipoRacionEnum,
		IDTerminal:     "LOCAL-1",
		HoraEvento:     time.Now().UnixMilli(),
		EstadoRegistro: db.Pendiente,
	}

	err = dbRepo.GuardarRegistroRacion(nuevoRegistro)
	if err != nil {
		f.Printf("(-) [GO]: Error guardando ticket en DB: %v\n", err)
		return
	}

	// 7. Emitir el ticket físico/TXT
	err = ticket.EmitirTicket(*perfil, fechaTXT, racion)
	if err != nil {
		f.Printf("(-) [GO]: Error emitiendo ticket: %v\n", err)
		return
	}

	f.Printf("(+) [TICKET]: Ticket generado y guardado para %s (%s)\n", perfil.NombreCompleto, racion)
}
