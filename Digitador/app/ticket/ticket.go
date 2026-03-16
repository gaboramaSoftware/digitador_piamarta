package ticket

import (
	db "Pydigitador/core/db"
	"fmt"
	f "fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"
)

// EmitirTicket genera un ticket de cambio en un archivo de texto.
// Retorna un error si algo falla, así el llamador puede manejarlo.
func EmitirTicket(p db.PerfilEstudiante, fecha string, racion string) error {
	// 1. Generar nombre de archivo único
	timestamp := time.Now().UnixMilli() // milisegundos desde epoch

	// 2. Construir ruta relativa al ejecutable
	// Obtenemos el directorio del ejecutable actual
	exePath, err := os.Executable()
	if err != nil {
		exePath = "." // fallback al directorio actual
	}
	baseDir := filepath.Join(filepath.Dir(exePath), "tickets")

	// Creamos la carpeta si no existe
	if err := os.MkdirAll(baseDir, 0755); err != nil {
		return fmt.Errorf("no se pudo crear la carpeta de tickets: %w", err)
	}

	filename := filepath.Join(baseDir, fmt.Sprintf("ticket_%s_%d.txt", p.RunID, timestamp))

	// 3. Crear archivo
	file, err := os.Create(filename)
	if err != nil {
		return fmt.Errorf("no se pudo crear el archivo de ticket: %w", err)
	}
	defer file.Close()

	// 4. Truncar nombre completo a 24 caracteres (manejando UTF-8)
	nombreTruncado := truncarString(p.NombreCompleto, 24)

	// 5. Preparar contenido usando los campos de db.PerfilEstudiante
	contenido := fmt.Sprintf("             TICKET             \n"+
		"NOMBRE: %s\n"+
		"CURSO:  %s %s\n"+
		"FECHA:  %s\n"+
		"RACION: %s",
		nombreTruncado, p.Curso, p.Letra, fecha, racion)

	// 6. Escribir al archivo
	_, err = file.WriteString(contenido)
	if err != nil {
		return f.Errorf("error escribiendo ticket: %w", err)
	}

	// 7. Mensaje de éxito
	f.Printf("\n[SISTEMA] Ticket guardado en: %s\n", filename)

	// 8. Enviar a imprimir usando la impresora predeterminada de Windows
	err = ImprimirTicketWindows(filename)
	if err != nil {
		f.Printf("(-) Error al intentar imprimir: %v\n", err)
	}

	return nil
}

// truncarString corta un string a max caracteres (runas), evitando romper una runa.
func truncarString(s string, max int) string {
	runas := []rune(s)
	if len(runas) > max {
		return string(runas[:max])
	}
	return s
}

// ImprimirTicketWindows usa powershell para enviar el archivo al driver de impresión de Windows
// (Equivalente al ShellExecuteA con el verbo "print" usado en C++)
func ImprimirTicketWindows(filepath string) error {
	f.Printf("[SISTEMA] Enviando a imprimir (Driver Windows): %s...\n", filepath)

	// Invocamos a powershell para usar Start-Process con -Verb Print
	// Esto manda a imprimir el archivo .txt con el programa predeterminado (usualmente Notepad)
	cmdStr := f.Sprintf("Start-Process -FilePath '%s' -Verb Print -WindowStyle Hidden", filepath)
	cmd := exec.Command("powershell", "-WindowStyle", "Hidden", "-Command", cmdStr)

	err := cmd.Run()
	if err != nil {
		return err
	}
	f.Println("(+) Orden enviada al driver de Windows.")
	return nil
}
