package enroll

import (
	"bufio"
	f "fmt"
	"os"
	"strings"
	"unicode"
)

// Creamos funcion para limpiar el string de nombres
func formatoNombre(s string) string {
	s = strings.TrimSpace(s)
	if s == "" {
		return ""
	}
	palabras := strings.Fields(s)
	for i, p := range palabras {
		if len(p) == 0 {
			continue
		}
		runas := []rune(p)
		runas[0] = unicode.ToUpper(runas[0])
		for j := 1; j < len(runas); j++ {
			runas[j] = unicode.ToLower(runas[j])
		}
		palabras[i] = string(runas)
	}
	return strings.Join(palabras, " ")
}

// Creamos funcion para recibir los nombres del estudiante usando un scanner global o pasándolo por parámetro.
// Para arreglar el bug de buffering, crearemos el scanner una sola vez.
var consolaScanner = bufio.NewScanner(os.Stdin)

func LeerNombreCompleto() (nombres, paterno, materno string) {
	//le pedimos al usuario que ingrese los nombres del alumno
	f.Print("Nombres del alumno: ")
	// Ignorar lecturas vacías (saltos de línea huérfanos de lecturas anteriores)
	for consolaScanner.Scan() {
		nombres = strings.TrimSpace(consolaScanner.Text())
		if nombres != "" {
			break
		}
		// si estaba vacío seguimos esperando
	}
	//guardamos los nombres limpios en la variable
	nombres = formatoNombre(nombres)

	//pedimos y guardamos el apellido paterno
	f.Print("Apellido Paterno: ")
	consolaScanner.Scan()
	paterno = formatoNombre(consolaScanner.Text())

	//pedimos y guardamos el apellido materno
	f.Print("Apellido Materno (dejar en blanco si o tiene): ")
	consolaScanner.Scan()
	materno = formatoNombre(consolaScanner.Text())

	//Bloquear estudiante si no posee al menos un apellido
	// (art. 31 Ministerio de justicia)
	if strings.TrimSpace(paterno) == "" && strings.TrimSpace(materno) == "" {
		f.Println("no se puede añadir a un usuario sin apellido")
		return
	}

	return
}

// funcion para recibir el curso del alumno (ahora retorna ID numérico)
func LeerCurso() (idCurso int) {
	//le pedimos al usuario que ingrese el ID del curso
	f.Print("ID del Curso (número): ")
	consolaScanner.Scan()
	input := strings.TrimSpace(consolaScanner.Text())
	if input == "" {
		f.Fprintln(os.Stderr, "ERROR: Curso vacio.")
		return 0
	}
	_, err := f.Sscanf(input, "%d", &idCurso)
	if err != nil {
		f.Fprintln(os.Stderr, "ERROR: Debe ingresar un número válido para el curso.")
		return 0
	}
	return
}

// funcion para recibir la letra del curso (ahora retorna ID numérico)
func LeerLetra() (idLetra int) {
	f.Print("ID de Letra/Sección (número, 0 si no aplica): ")
	consolaScanner.Scan()
	input := strings.TrimSpace(consolaScanner.Text())
	if input == "" {
		return 0
	}
	f.Sscanf(input, "%d", &idLetra)
	return
}
