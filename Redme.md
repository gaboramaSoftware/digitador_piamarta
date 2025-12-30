# 🖐️ Digitador Piamarta - Sistema Biométrico

## 📋 Descripción

Sistema de verificación biométrica para el Colegio Piamarta, compuesto por:
- **Backend C++**: Servidor web con Crow en puerto 18080
- **Frontend Electron**: Interfaz kiosk para el totem
- **Sensor ZKTeco**: Lector de huellas digitales

---

## 📁 Estructura de Archivos

```
C:\Digitador\
├── src\
│   ├── Digitador.exe        # Servidor C++ compilado
│   ├── main.cpp             # Código fuente
│   ├── BD\                  # Base de datos SQLite
│   └── asio-1.30.2\         # Librería de networking
│
├── totem\
│   ├── main.js              # Proceso principal Electron
│   ├── preload.js           # Script de precarga
│   ├── index.html           # Vista principal
│   └── package.json         # Dependencias Node
│
├── bin\
│   ├── libzkfp.dll          # SDK ZKTeco
│   ├── libcrypto-3-x64.dll  # OpenSSL
│   └── libssl-3-x64.dll     # OpenSSL
│
├── logs\                    # Logs del sistema
│
├── IniciarDigitador.bat     # ⭐ Script principal de inicio
├── IniciarDigitador.ps1     # Script PowerShell (avanzado)
├── DetenerDigitador.bat     # Detener el sistema
├── EstadoSistema.bat        # Ver estado actual
└── Instalar.bat             # Instalación inicial
```

---

## 🚀 Inicio Rápido

### Opción 1: Doble clic (Recomendado)
```
Doble clic en: IniciarDigitador.bat
```

### Opción 2: PowerShell (Avanzado)
```powershell
powershell -ExecutionPolicy Bypass -File IniciarDigitador.ps1
```

### Opción 3: Modo Kiosk (Pantalla completa)
```powershell
powershell -ExecutionPolicy Bypass -File IniciarDigitador.ps1 -Kiosk
```

---

## ⚙️ Instalación en NUC Nuevo

### 1. Prerequisitos
- Windows 10/11 (64 bits)
- Node.js v18+ ([descargar](https://nodejs.org/))
- Drivers ZKTeco instalados

### 2. Copiar archivos
```cmd
xcopy /E /I "\\servidor\digitador" "C:\Digitador"
```

### 3. Ejecutar instalador
```cmd
C:\Digitador\Instalar.bat
```

El instalador:
- ✅ Verifica Node.js y npm
- ✅ Crea directorios necesarios
- ✅ Instala dependencias npm
- ✅ Crea acceso directo en escritorio
- ✅ Configura inicio automático (opcional)

---

## 🔧 Configuración

### Cambiar puerto del servidor
Editar `IniciarDigitador.bat`:
```batch
SET "SERVER_PORT=18080"
```

### Cambiar directorio de instalación
Editar `IniciarDigitador.bat`:
```batch
SET "INSTALL_DIR=C:\Digitador"
```

### Timeout de conexión
Editar `IniciarDigitador.bat`:
```batch
SET "MAX_WAIT_SECONDS=60"
```

---

## 🔍 Diagnóstico

### Ver estado del sistema
```cmd
EstadoSistema.bat
```

### Ver logs del servidor
```cmd
type C:\Digitador\logs\server_*.log
```

### Probar API manualmente
```cmd
curl http://localhost:18080/api/sensor/status
```

### Verificar procesos
```cmd
tasklist | findstr "Digitador electron"
```

---

## 🛠️ Solución de Problemas

### ❌ "No se encontró Digitador.exe"
- Verifica que el archivo existe en `C:\Digitador\src\`
- Recompila el proyecto si es necesario

### ❌ "Timeout esperando al servidor"
1. Revisa los logs: `C:\Digitador\logs\`
2. Verifica que el puerto 18080 no está en uso:
   ```cmd
   netstat -an | findstr 18080
   ```
3. Verifica que las DLLs están en `C:\Digitador\bin\`

### ❌ "npm no encontrado"
1. Instala Node.js desde https://nodejs.org/
2. Reinicia la terminal
3. Verifica: `node --version`

### ❌ Sensor no detectado
1. Verifica conexión USB del sensor
2. Instala drivers ZKTeco
3. Verifica DLL: `C:\Digitador\bin\libzkfp.dll`

---

## 📊 API Endpoints

| Endpoint | Método | Descripción |
|----------|--------|-------------|
| `/api/sensor/status` | GET | Estado del sensor |
| `/api/alumno/verificar` | POST | Verificar huella |
| `/api/alumno/enrolar` | POST | Registrar alumno |
| `/api/ticket/procesar` | POST | Procesar ticket |

---

## 🔄 Inicio Automático con Windows

### Opción 1: Durante instalación
El script `Instalar.bat` pregunta si deseas inicio automático.

### Opción 2: Manual
1. Presiona `Win + R`
2. Escribe: `shell:startup`
3. Copia `IniciarDigitador.bat` a esa carpeta

### Opción 3: Tarea programada
```cmd
schtasks /create /tn "Digitador" /tr "C:\Digitador\IniciarDigitador.bat" /sc onlogon /rl highest
```

---

## 📝 Logs

Los logs se guardan en `C:\Digitador\logs\` con formato:
```
server_YYYYMMDD.log
```

Para ver logs en tiempo real:
```powershell
Get-Content C:\Digitador\logs\server_*.log -Tail 50 -Wait
```

---

## 👥 Soporte

- **Desarrollador**: GaboramaSoftware
- **Repositorio**: https://github.com/gaboramaSoftware/digitador_piamarta

---

## 📜 Licencia

Proyecto privado para Colegio Piamarta.