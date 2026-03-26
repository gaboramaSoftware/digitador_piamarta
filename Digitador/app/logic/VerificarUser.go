// VerificarUser identifica a un usuario por su huella dactilar
// A diferencia de VerificarDuplicidad (en enroll), esta función
// busca al usuario para mostrarlo, no para rechazar duplicados.
package logic

import (
	f "fmt"
	"os"

	sensor "Pydigitador/core/Hardware/Sensor"
	db "Pydigitador/core/db"
)

// VerificarUsuario busca la huella capturada contra todos los templates
// de la base de datos y muestra los datos del usuario si lo encuentra.
func VerificarUsuario(sensorAdapter *sensor.SensorAdapter, tplCapturado []byte, database db.DB) error {
	// obtener todos los templates desde la base de datos
	allTemplates, err := database.ObtenerTodosTemplates()
	if err != nil {
		return f.Errorf("no se encuentran templates en la base de datos: %w", err)
	}

	// recorre el mapa de templates
	f.Printf("    [DEBUG] Templates a comparar: %d\n", len(allTemplates))
	for run, existingTpl := range allTemplates {
		// compara el score de cada una
		score, err := sensorAdapter.CompararHuellas(tplCapturado, existingTpl)
		// hay templates corruptos
		if err != nil {
			f.Fprintf(os.Stderr, "(-)[GO]: error al comparar el run %s: %v\n", run, err)
			continue
		}

		// hay huellas identicas?
		if score >= db.MatchThreshold {
			// si, mostramos los datos del usuario encontrado
			f.Printf("(+) [VERIFICADOR]: Estudiante encontrado en el sistema\n")
			perfil, err := database.ObtenerPerfilPorRunID(run)
			if err == nil && perfil != nil {
				f.Printf("    RUN: %s-%s\n", perfil.RunID, perfil.DV)
				f.Printf("    Nombre: %s\n", perfil.NombreCompleto)
				f.Printf("    Curso: %s %s\n", perfil.Curso, perfil.Letra)
			} else {
				f.Printf("    RUN: %s\n", run)
				f.Printf("    (!) Error obteniendo perfil completo: %v\n", err)
			}
			return nil // encontrado, salimos
		}
	}
	// si llegamos hasta acá, no se encontro la huella con un score suficiente
	f.Printf("(-) [VERIFICADOR]: Estudiante no está registrado en el sistema\n")
	f.Printf("(-) [VERIFICADOR]: Para enrolar la huella, diríjase al menú de enrolamiento\n")
	return nil
}
