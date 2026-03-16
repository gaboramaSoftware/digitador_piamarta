// este archivo se encarga de iniciar el servidor de electron
// y controlar los eventos de la aplicacion

package infra

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
)

func ElectronTotem() {
	fmt.Println("Iniciando ElectronTotem...")
	// Usamos la ruta relativa al proyecto
	totemPath := "./infra/totem"
	if _, err := os.Stat(totemPath); os.IsNotExist(err) {
		// Fallback para cuando se ejecuta desde cmd/ o bin/
		totemPath = "../infra/totem"
	}
	
	// Obtenemos la ruta absoluta para asegurarnos de que electron la encuentre
	absPath, err := filepath.Abs(totemPath)
	if err != nil {
		fmt.Printf("(!) Error determinando la ruta absoluta del totem: %v\n", err)
		return
	}

	// Intentamos ejecutar electron con la ruta absoluta
	cmd := exec.Command("npx", "electron", absPath)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	err = cmd.Start()
	if err != nil {
		fmt.Printf("(!) Error al iniciar ElectronTotem: %v\n", err)
		return
	}

	fmt.Println("(+) ElectronTotem iniciado en segundo plano.")
}
