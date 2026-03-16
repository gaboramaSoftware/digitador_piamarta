package infra

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	logic "Pydigitador/app/logic"
	digitador "Pydigitador/core/Hardware/Sensor"
	repository "Pydigitador/infra/DB"
	"Pydigitador/infra/web"
)

// limpiarPantalla limpia la consola en Windows
func limpiarPantalla() {
	cmd := exec.Command("cmd", "/c", "cls")
	cmd.Stdout = os.Stdout
	cmd.Run()
}

// pausa espera a que el usuario presione Enter
func pausa() {
	fmt.Println("\nPresione 'Enter' para continuar...")
	bufio.NewReader(os.Stdin).ReadBytes('\n')
}

func Main() {
	running := true

	// inicializamos la base de datos
	//no podriamos mejor iniciar la base de datos importandola en el main.go
	//respuesta
	exePath, err := os.Executable()
	if err != nil {
		fmt.Fprintf(os.Stderr, "(-) [GO]: Error obteniendo la ruta del ejecutable: %v\n", err)
		return
	}
	exeDir := filepath.Dir(exePath)
	dbPath := filepath.Join(exeDir, "core", "DB", "digitador.db")

	// Fallback para desarrollo (cuando usamos "go run" el ejecutable se crea en una carpeta temporal)
	if _, err := os.Stat(dbPath); os.IsNotExist(err) {
		dbPath = "./core/DB/digitador.db"
		// Si estamos corriendo "go run ." dentro de la carpeta "cmd"
		if _, err := os.Stat(dbPath); os.IsNotExist(err) {
			dbPath = "../core/DB/digitador.db"
		}
	}

	dbRepo, err := repository.NewSQLiteUserRepository(dbPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "(-) [GO]: Error iniciando la base de datos en %s: %v\n", dbPath, err)
		return
	}
	defer dbRepo.Close()

	fmt.Println("(+) [GO]: Base de datos conectada correctamente.")
	limpiarPantalla()

	// inicializamos el sensor de huellas
	sensor, err := digitador.SensorGO()
	if err != nil {
		fmt.Fprintf(os.Stderr, "(-) [GO]: Error iniciando sensor: %v\n", err)
		fmt.Println("(!) El sistema continuará sin sensor de huellas.")
		pausa()
	} else {
		defer sensor.Cerrar()
	}

	// modo totem: arranca directo sin menu (usado en produccion)
	for _, arg := range os.Args[1:] {
		if arg == "--totem" {
			fmt.Println("(+) Modo totem activado. Iniciando servidor y totem...")
			go web.StartApiServer(8080, sensor, dbRepo)
			ElectronTotem()
			return
		}
	}

	// variable para la opcion del menu
	var opt int

	// inicializamos menu
	for running {
		presentacion()
		fmt.Scan(&opt)
		// consumir el \n restante del Scan
		bufio.NewReader(os.Stdin).ReadBytes('\n')

		switch opt {
		case 1:
			logic.EnrollUsers(sensor, dbRepo)
			pausa()
			limpiarPantalla()
		case 2:
			logic.MenuVerificar(sensor, dbRepo)
			pausa()
			limpiarPantalla()
		case 3:
			logic.MenuTicket(sensor, dbRepo)
			pausa()
			limpiarPantalla()
		case 4:
			fmt.Println("(+) Iniciando servidor web para el Totem y la Dashboard...")
			// r es nuestro dbRepo y sensor es el objeto sensor
			// Iniciamos el nuevo servidor API Rest unificado
			go web.StartApiServer(8080, sensor, dbRepo)

			fmt.Println("(!) El totem iniciará. El sensor está integrado.")
			ElectronTotem()
			pausa()
			limpiarPantalla()
		case 5:
			logic.MenuRegisterRecient(dbRepo)
			pausa()
			limpiarPantalla()
		case 6:
			logic.MenuShowRecent(dbRepo)
			pausa()
			limpiarPantalla()
		case 7:
			if dbRepo.BorrarRegistro() {
				fmt.Println("Registros de raciones eliminados correctamente.")
			} else {
				fmt.Println("Hubo un error al intentar borrar los registros.")
			}
			pausa()
			limpiarPantalla()
		case 8:
			if dbRepo.BorrarUsuario() {
				fmt.Println("Registros de raciones eliminados correctamente.")
			} else {
				fmt.Println("Hubo un error al intentar borrar los registros.")
			}
			pausa()
			limpiarPantalla()
		case 9:
			running = false
			limpiarPantalla()
		case 10:
			fmt.Print("\n ADVERTENCIA: ¿está seguro de borrar TODOS los datos? (s/n): ")
			var r1 string
			fmt.Scan(&r1)
			if strings.ToLower(r1) == "s" {
				fmt.Print("¿SEGURO SEGURO? Escribe BORRAR en la terminal: ")
				var r2 string
				fmt.Scan(&r2)
				if strings.ToLower(r2) == "borrar" {
					fmt.Println("Borrando...")
					dbRepo.BorrarTodo()
					fmt.Println("Hecho.")
				} else {
					fmt.Println("Cancelado.")
				}
			} else {
				fmt.Println("Cancelado.")
			}
			pausa()
			limpiarPantalla()
		default:
			fmt.Println("Opción inválida. Intente nuevamente.")
			limpiarPantalla()
		}
	}

	fmt.Println("Programa terminado.")
}
