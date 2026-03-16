# Guía de Instalación — Digitador Piamarta

Este documento detalla el proceso para compilar, preparar y desplegar el sistema Digitador Piamarta en una unidad NUC (Tótem).

---

## 📋 Requisitos Previos

### En Máquina de Desarrollo (Windows)
- **Go 1.25+**
- **MinGW-w64** (GCC para compilar CGO/SQLite3) — Debe estar en el `PATH`.
- **Node.js 22+**
- **Python 3.12+**
- **Librerías Python**: `pip install pandas openpyxl`
- **SDK ZKTeco**: El archivo `libzkfp.dll` debe estar en `Digitador\core\Hardware\Sensor\x64lib\`.

### En NUC (Producción)
- **Privilegios de Administrador**.
- Instaladores de **Node.js (.msi)** y **Python (.exe)** (si no están instalados).

---

## 🛠️ Fase 1: Preparación del Paquete (Build)

En la máquina de desarrollo, abre una terminal (PowerShell) en la raíz del proyecto y ejecuta:

```powershell
.\scripts\build_deploy.ps1
```

**¿Qué hace este script?**
1. Verifica que tienes Go, GCC, Node y Python instalados.
2. Compila el binario `digitador.exe`.
3. Descarga las dependencias de **Electron** para el Tótem (UI).
4. Empaqueta todo (binario, DB, Python, UI y scripts) en un archivo llamado `digitador_deploy.zip` en la raíz del proyecto.

---

## 🚀 Fase 2: Instalación en el NUC

1. **Copiar archivos**: Lleva el archivo `digitador_deploy.zip` al NUC (vía Pendrive o red).
2. **Descargar Instaladores**: Asegúrate de incluir los instaladores de Node.js y Python en la misma carpeta donde extraerás el ZIP (esto permite al script instalarlos si faltan).
3. **Extraer**: Descomprime el contenido en una carpeta temporal.
4. **Ejecutar Setup**:
   - Abre PowerShell como **Administrador**.
   - Navega a la carpeta extraída.
   - Ejecuta:
     ```powershell
     .\setup_nuc.ps1
     ```

**Durante el proceso:**
- El script instalará automáticamente Node.js y Python si no se encuentran.
- Copiará todos los archivos a `C:\Digitador`.
- Registrará una tarea en el **Task Scheduler** de Windows llamada `DigitadorTotem` para que el sistema inicie automáticamente al iniciar sesión.

---

## 🖥️ Fase 3: Configuración Modo Kiosko (Opcional)

Al final del script `setup_nuc.ps1`, se te preguntará si deseas configurar el **Inicio de Sesión Automático**:
1. Ingresa el nombre de usuario de Windows.
2. Ingresa la contraseña de dicho usuario.
3. El NUC ahora entrará al escritorio sin pedir contraseña, y el Tótem se abrirá solo.

---

## 🔍 Verificación

- **Reinicio**: Reinicia el equipo. El Tótem debería verse en pantalla completa una vez cargue el escritorio.
- **Base de Datos**: El sistema buscará automáticamente un archivo Excel para poblar la base de datos si es la primera vez.
- **Logs**: Si algo falla, revisa el Visor de Eventos de Windows (`Event Viewer`) > `Windows Logs` > `Application`.

---

## 🔐 Credenciales del Panel Web

Para acceder al Dashboard administrativo (`http://localhost:8080`):
- **URL**: `http://localhost:8080`
- **RUT**: `111111111`
- **Contraseña**: `admin123`

---

## 🗑️ Desinstalación

Para eliminar la tarea de inicio automático, ejecuta como Administrador:
```powershell
Unregister-ScheduledTask -TaskName "DigitadorTotem" -Confirm:$false
```
Y luego elimina la carpeta `C:\Digitador`.
