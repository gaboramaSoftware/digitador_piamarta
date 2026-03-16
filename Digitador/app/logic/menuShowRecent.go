// menuShowRecent muestra los últimos registros de ración de la base de datos
package logic

import (
	f "fmt"

	repo "Pydigitador/infra/DB"
)

// MenuShowRecent muestra los últimos registros almacenados en la base de datos
func MenuShowRecent(dbRepo *repo.SQLiteUserRepository) {
	registros, err := dbRepo.ObtenerUltimosRegistros()
	if err != nil {
		f.Printf("(-) [GO]: Error obteniendo registros: %v\n", err)
		return
	}

	if len(registros) == 0 {
		f.Println("(!) No hay registros guardados aún.")
		return
	}

	f.Println("\n========= ÚLTIMOS REGISTROS =========")
	for i, reg := range registros {
		f.Printf("%d) ID: %d | Estudiante: %s | Fecha: %s | Ración: %d\n",
			i+1,
			reg.IDRegistro,
			reg.IDEstudiante,
			reg.FechaServicio,
			reg.TipoRacion,
		)
	}
	f.Println("=====================================")
}

// MenuRegisterRecient muestra los ultimos alumnos que han sido almacenados en la base de datos
func MenuRegisterRecient(dbRepo *repo.SQLiteUserRepository) {
	registros, err := dbRepo.ObtenerUltimosRegistros()
	if err != nil {
		f.Printf("(-) [GO]: Error obteniendo registros: %v\n", err)
		return
	}

	if len(registros) == 0 {
		f.Println("(!) No hay registros guardados aún.")
		return
	}

	f.Println("\n========= ÚLTIMOS REGISTROS =========")
	for i, reg := range registros {
		f.Printf("%d) ID: %d | Estudiante: %s | Fecha: %s | Ración: %d\n",
			i+1,
			reg.IDRegistro,
			reg.IDEstudiante,
			reg.FechaServicio,
			reg.TipoRacion,
		)
	}
	f.Println("=====================================")
}
