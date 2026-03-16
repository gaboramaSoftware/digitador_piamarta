// MenuVerificar orquesta el flujo de verificación de huella desde el menú principal:
// captura la huella del sensor y luego busca al usuario en la base de datos.
package logic

import (
	f "fmt"
	"time"

	digitador "Pydigitador/core/Hardware/Sensor"
	repo "Pydigitador/infra/DB"
)

// MenuVerificar es el punto de entrada desde el menú para el flujo de verificación.
func MenuVerificar(sensor *digitador.SensorAdapter, dbRepo *repo.SQLiteUserRepository) {
	// ¿hay sensor disponible?
	if sensor == nil {
		f.Println("(!) Sensor no disponible para verificar.")
		return
	}

	f.Println("Coloque su dedo en el sensor (tiene 10 segundos)...")

	// 1. Capturar huella con timeout
	plantilla, err := sensor.CapturarHuellaTimeout(10 * time.Second)
	if err != nil {
		f.Printf("(-) [GO]: No se pudo capturar la huella: %v\n", err)
		return
	}

	// 2. Buscar en la base de datos
	err = VerificarUsuario(sensor, plantilla, dbRepo)
	if err != nil {
		f.Printf("(-) [GO]: Error al verificar usuario: %v\n", err)
	}
}
