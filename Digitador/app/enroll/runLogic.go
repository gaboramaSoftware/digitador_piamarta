// logica del rut del usuario
package enroll

import (
	"fmt"
	f "fmt"
	"os"
	"strconv"
	"unicode"

	"golang.org/x/term"
)

var rawRun string

func pausa() {
	var dummy string
	f.Println("Presione 'Enter' para continuar...")
	f.Scanln(&dummy)
}

// creamos la funcion para ordenar el run
func FormatoRun() (uint64, string, bool) {
	//le pedimos al usuario que ingrese el rut/ipe
	f.Println("Ingrese el RUT/IPE del estudiante: ")

	//usamos la consola en modo interactivo para capturar teclas
	//directamente desde la ram
	oldState, err := term.MakeRaw(int(os.Stdin.Fd()))

	//hay errores de digitacion?
	if err != nil {
		//si capturar error
		f.Fprintln(os.Stderr, "(-) [GO]: error configurando la terminal", err)
		return 0, "", false
	}

	//guardamos los digitos capturados en rawRut
	//(rune es el alias para int 32)
	var rawRun []rune

	//creamos un ciclo for para ordenar el rut
	for {
		b := make([]byte, 1)
		_, err := os.Stdin.Read(b)
		if err != nil {
			term.Restore(int(os.Stdin.Fd()), oldState)
			return 0, "", false
		}
		ch := rune(b[0])
		if ch == 27 { // ESC -> para salir
			term.Restore(int(os.Stdin.Fd()), oldState)
			f.Println("\n[Cancelado]")
			return 0, "", false
		}
		if ch == '\r' || ch == '\n' { // ENTER -> terminar captura
			term.Restore(int(os.Stdin.Fd()), oldState)
			f.Println()
			break
		}
		if (ch == '\b' || ch == 127) && len(rawRun) > 0 { // RETROCESO -> para borrar
			rawRun = rawRun[:len(rawRun)-1]
			f.Print("\b \b")
			continue
		}
		// Aceptar solo dígitos, K y guión
		if unicode.IsDigit(ch) || ch == 'K' || ch == '-' || ch == 'k' {
			rawRun = append(rawRun, ch)
			f.Print(string(ch)) // Echo al usuario
		}
	}

	//convertimos el run a string, quitando el guion y pasando a mayúscula
	var cleanRunStr string
	for _, r := range rawRun {
		if r != '-' {
			cleanRunStr += string(unicode.ToUpper(r))
		}
	}

	//el tamaño del run es muy pequeño?
	if len(cleanRunStr) < 2 {
		//si, retornamos error
		fmt.Fprintln(os.Stderr, "ERROR: RUT demasiado corto.")
		// Equivale al system("PAUSE")
		pausa()
		return 0, "", false
	}

	//separamos el dv del run
	dv := string(cleanRunStr[len(cleanRunStr)-1])
	//quitamos el dv del run
	cuerpoStr := cleanRunStr[:len(cleanRunStr)-1]

	//convertimos el cuerpo a numero
	cuerpo, err := strconv.ParseUint(cuerpoStr, 10, 64)
	if err != nil {
		f.Fprintln(os.Stderr, "ERROR: Formato de RUT invalido (cuerpo no numerico).")
		pausa()
		return 0, "", false
	}

	//devolvemos los datos limpios para calcular el M-11
	return cuerpo, dv, true
}

// creamos una funcion para calcular el algoritmo de m-11
func CalcularM11(cuerpo uint64, dv string) string {
	//creamos variables de suma y multiplicacion
	var suma uint64 = 0
	var multi uint64 = 2

	//creamos ciclo for para m11 (no me pregunten como funciona la saque de internet 👍)
	for cuerpo > 0 {
		suma += (cuerpo % 10) * multi
		cuerpo /= 10
		multi++
		if multi > 7 {
			multi = 2
		}
	}

	// calculamos el resto DESPUES del ciclo (antes estaba mal!)
	resto := 11 - (suma % 11)

	//el resto es igual a 11?
	if resto == 11 {
		return "0"
	}
	//el resto es igual a 10
	if resto == 10 {
		return "K"
	}
	//devolvemos el run verificado por el m11
	return f.Sprintf("%d", resto)
}
