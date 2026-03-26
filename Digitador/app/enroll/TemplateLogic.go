package enroll

import (
	f "fmt"
	"os"

	//importar sensorAdapter

	db "Pydigitador/core/db"
	sensor "Pydigitador/core/Hardware/Sensor" //sensor adapter no sensor C
)

// verificamos la duplicidad de la huella dactilar
func VerificarDuplicidad(sensorAdapter *sensor.SensorAdapter, tplCapturado []byte, database db.DB) error {
	//obtener todos los templates desde la base de datos
	allTemplates, err := database.ObtenerTodosTemplates()

	if err != nil {
		return f.Errorf("No se encuentran templates en la base de datos: %w", err)
	}

	//recorre el mapa de templates
	for run, existingTpl := range allTemplates {
		//compara el score de cada una
		score, err := sensorAdapter.CompararHuellas(tplCapturado, existingTpl)
		//hay templates corruptos
		if err != nil {
			f.Fprintf(os.Stderr, "(-)[GO]: error al comparar el run %s: %v\n", run, err)
			continue
		}

		//hay huellas identicas?
		if score >= db.MatchThreshold {
			//si, devolvemos error de huella duplicada
			f.Fprintf(os.Stderr, "(-) ERROR: La huella esta registrada dentro del sistema\n")
			nombreAlumno := "Desconocido"
			if user, err := database.GetUser(run); err == nil && user != nil {
				nombreAlumno = user.NombreCompleto
			}
			f.Fprintf(os.Stderr, "    Pertenece al RUN: %s\n", run)
			f.Fprintf(os.Stderr, "	  Nombre de alumno: %s\n", nombreAlumno)
			f.Fprintf(os.Stderr, "    No se puede enrolar la misma huella dos veces.\n")
			return f.Errorf("huella duplicada con el run: %s", run)
		}
	}

	return nil
}
