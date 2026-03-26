package infra

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"

	logic "Pydigitador/app/logic"
	digitador "Pydigitador/core/Hardware/Sensor"
	repository "Pydigitador/infra/DB"
	"Pydigitador/infra/web"
)

// limpiarPantalla limpia la consola en Windows
func limpiarPantalla() {
	var cmd *exec.Cmd
	if runtime.GOOS == "windows" {
		// En Windows, 'cls' es un comando interno de cmd.exe
		cmd = exec.Command("cmd", "/c", "cls")
	} else {
		// En Linux/macOS, 'clear' es un ejecutable independiente
		cmd = exec.Command("clear")
	}

	// Es vital conectar la salida del comando a la salida estándar actual
	cmd.Stdout = os.Stdout
	cmd.Run()
}

// pausa espera a que el usuario presione Enter
func pausa() {
	fmt.Println("\nPresione 'Enter' para continuar...")
	bufio.NewReader(os.Stdin).ReadBytes('\n')
	limpiarPantalla()
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
		limpiarPantalla()
		presentacion()
		fmt.Scan(&opt)
		// consumir el \n restante del Scan
		bufio.NewReader(os.Stdin).ReadBytes('\n')
		limpiarPantalla()

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
			if dbRepo.BorrarRegistro() {
				fmt.Println("Registros de raciones eliminados correctamente.")
			} else {
				fmt.Println("Hubo un error al intentar borrar los registros.")
			}
			pausa()
			limpiarPantalla()
		case 6:
			if dbRepo.BorrarUsuario() {
				fmt.Println("Registros de raciones eliminados correctamente.")
			} else {
				fmt.Println("Hubo un error al intentar borrar los registros.")
			}
			pausa()
			limpiarPantalla()
		case 7:
			running = false
			limpiarPantalla()
		case 8:
			fmt.Print("Ingrese la ruta del archivo Excel: ")
			reader := bufio.NewReader(os.Stdin)
			path, _ := reader.ReadString('\n')
			path = strings.TrimSpace(path)

			err := dbRepo.SubirPlantillaExcel(path, false)
			if err != nil {
				if err.Error() == "CONFLICT_STUDENTS" {
					fmt.Print("(!) Alumnos detectados. ¿Sobrescribir datos (manteniendo huellas)? (s/n): ")
					var conf string
					fmt.Scan(&conf)
					if strings.ToLower(conf) == "s" {
						err = dbRepo.SubirPlantillaExcel(path, true)
						if err != nil {
							fmt.Printf("(-) Error: %v\n", err)
						} else {
							fmt.Println("(+) Excel cargado con éxito.")
						}
					}
				} else {
					fmt.Printf("(-) Error: %v\n", err)
				}
			} else {
				fmt.Println("(+) Excel cargado con éxito.")
			}
			pausa()
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
