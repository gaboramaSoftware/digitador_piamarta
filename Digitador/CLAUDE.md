# Contexto del Proyecto: Pydigitador

## Detalles importantes
- **Stack tecnológico**: 
  - **Backend**: Go 1.24/1.25 (Orquestación, Lógica de Negocio).
  - **Frontend (Tótem)**: Electron ^40.8.0 (JavaScript, HTML, CSS).
  - **Hardware**: C/C++ (Integración con sensores biométricos vía `cgo` y `libzkfp`).
  - **Base de Datos**: SQLite (Persistence layer via `go-sqlite3`).
  - **Comunicación**: HTTP/REST API interna (`net/http`).
- **Arquitectura**: Hexagonal (Puertos y Adaptadores).
  - `core/`: Entidades, Modelos de Hardware (Abstracción) y Lógica de Dominio.
  - `app/`: Casos de uso (Lógica de aplicación: Enrolamiento, Verificación).
  - `infra/`: Adaptadores (Base de Datos, Servidor Web, Lanzador de Electron).
- **Convenciones de código**: 
  - Comentarios y nombres de funciones/variables predominantemente en **Español**.
  - Uso de **PascalCase** para símbolos exportados en Go.
  - Gestión de concurrencia con `sync.Mutex` para acceso seguro al hardware.
  - Manejo de errores idiomático de Go (`if err != nil`).

## Comandos Útiles
- **Ejecutar proyecto**: `go run cmd/main.go`
- **Instalar dependencias Electron**: `cd totem; npm install`
- **Compilar Go**: `go build -o pydigitador.exe cmd/main.go`
- **Tests**: `go test ./...`

## Estructura del proyecto
- `app/`: Lógica de enrolamiento y verificación de usuarios.
- `cmd/`: Punto de entrada principal.
- `core/`: Abstracción de Hardware (Sensores, Impresoras) y Modelos DB.
- `infra/`: Implementaciones de DB, Web Server y Bridge con Electron.
- `totem/`: Código fuente de la interfaz Electron.

## Avisos especiales
- Requiere librerías nativas (`libzkfp`) en el path para la integración del sensor.
- Configuración de `cgo` necesaria para compilar con C++.
- El tótem se inicia en segundo plano vía `npx electron ./totem`.
```

También crea un archivo `.claudeignore` para excluir archivos sensibles y grandes que no necesitas: 
```
node_modules/
.git/
.env
*.log
dist/
build/