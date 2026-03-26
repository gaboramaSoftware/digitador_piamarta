// gorutine para enrolar usuarios nuevos
package logic

import (
	f "fmt"
	"time"

	"Pydigitador/app/enroll"
	db "Pydigitador/core/db"
	digitador "Pydigitador/core/Hardware/Sensor"
	repo "Pydigitador/infra/DB"
)

// EnrollUsers registra un estudiante nuevo en el sistema
func EnrollUsers(sensor *digitador.SensorAdapter,
	dbRepo *repo.SQLiteUserRepository) {

	//le avisamos al usuario que esta enrolando
	f.Println("\n[Enrolamiento] Ingrese datos del estudiante:")

	// 1. Pedimos el RUT/IPE del estudiante
	cuerpoRun, dv, valid := enroll.FormatoRun()
	if !valid {
		f.Println("Hubo un error o se canceló el ingreso del RUN.")
		return
	}

	// 2. Calculamos y validamos el M11
	dvCalculado := enroll.CalcularM11(cuerpoRun, dv)
	if dvCalculado != dv {
		f.Println("(-) ERROR: El dígito verificador ingresado no es válido.")
		return
	}

	// 3. Pedimos el nombre completo al usuario
	nombres, paterno, materno := enroll.LeerNombreCompleto()
	if nombres == "" {
		return
	}

	// 4. Pedimos el curso al alumno (ahora retorna ID numérico)
	curso := enroll.LeerCurso()
	if curso == 0 {
		return
	}

	// 4.1 Pedimos la letra al alumno (ahora retorna ID numérico)
	letra := enroll.LeerLetra()

	// 5. Capturamos la huella dactilar usando el adaptador (OPCIONAL en el nuevo modelo)
	var plantilla []byte
	if sensor != nil {
		f.Println("¿Desea registrar huella? (s/n):")
		var respHuella string
		f.Scan(&respHuella)
		if respHuella == "s" || respHuella == "S" {
			f.Println("Coloque su dedo en el sensor (tiene 10 segundos)...")
			var err error
			plantilla, err = sensor.CapturarHuellaTimeout(10 * time.Second)
			if err != nil {
				f.Println(err)
				return
			}

			// 6. Verificamos la duplicidad de la huella
			err = enroll.VerificarDuplicidad(sensor, plantilla, dbRepo)
			if err != nil {
				f.Println("Error de duplicidad:", err)
				return
			}
		} else {
			f.Println("(!) Se registra alumno sin huella.")
		}
	} else {
		f.Println("(!) Sensor no disponible, se registra sin huella.")
	}

	// 7. Agrupamos todos los datos para registrarlos en base de datos
	f.Println("\n(+) [GO]: Datos recolectados correctamente. Registrando...")

	// Formateamos el nombre limpio
	nombreCompleto := f.Sprintf("%s %s %s", nombres, paterno, materno)

	// Creamos el objeto Usuario para guardar en la base de datos
	usuario := db.Usuario{
		RunID:          f.Sprintf("%d", cuerpoRun),
		DV:             dv,
		NombreCompleto: nombreCompleto,
		IDRol:          db.RolEstudiante,
		TemplateHuella: plantilla,
		Activo:         true, // Nuevo alumno siempre activo
	}

	// 8. Guardamos el usuario en la base de datos
	err := dbRepo.SaveUser(usuario)
	if err != nil {
		f.Printf("(-) [GO]: Error guardando usuario: %v\n", err)
		return
	}

	// 9. Guardamos curso y letra (con IDs numéricos)
	dbRepo.UpdateStudentCourse(usuario.RunID, curso, letra)

	f.Printf("(+) [GO]: Usuario %s registrado exitosamente.\n", nombreCompleto)
}
