# ============================================
# DIGITADOR PIAMARTA - LAUNCHER POWERSHELL
# ============================================
# Ejecutar con: powershell -ExecutionPolicy Bypass -File IniciarDigitador.ps1

param(
    [switch]$Kiosk,
    [switch]$NoUI,
    [switch]$Debug
)

$ErrorActionPreference = "Stop"

# ============================================
# CONFIGURACION
# ============================================
$Config = @{
    InstallDir = "C:\Digitador"
    ServerPort = 18080
    MaxWaitSeconds = 60
    ServerExe = "Digitador.exe"
    ServerArgs = "--webserver"
}

# Colores para output
function Write-Status($message, $type = "INFO") {
    $colors = @{
        "INFO" = "Cyan"
        "OK" = "Green"
        "WARN" = "Yellow"
        "ERROR" = "Red"
        "STEP" = "White"
    }
    $prefix = @{
        "INFO" = "[INFO]"
        "OK" = "[OK]"
        "WARN" = "[WARN]"
        "ERROR" = "[ERROR]"
        "STEP" = "[>>]"
    }
    Write-Host "$($prefix[$type]) $message" -ForegroundColor $colors[$type]
}

function Show-Banner {
    $banner = @"

  ____  _       _ _            _            
 |  _ \(_) __ _(_) |_ __ _  __| | ___  _ __ 
 | | | | |/ _` | | __/ _` |/ _` |/ _ \| '__|
 | |_| | | (_| | | || (_| | (_| | (_) | |   
 |____/|_|\__, |_|\__\__,_|\__,_|\___/|_|   
          |___/                              

  SISTEMA BIOMETRICO - COLEGIO PIAMARTA
  ==========================================

"@
    Write-Host $banner -ForegroundColor Cyan
}

function Test-ServerReady {
    param([int]$Port = 18080)
    
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:$Port/api/sensor/status" -TimeoutSec 2 -UseBasicParsing -ErrorAction Stop
        return $response.StatusCode -eq 200
    }
    catch {
        return $false
    }
}

function Stop-ExistingProcesses {
    Write-Status "Limpiando procesos anteriores..." "STEP"
    
    $processes = @("Digitador", "electron")
    foreach ($proc in $processes) {
        $running = Get-Process -Name $proc -ErrorAction SilentlyContinue
        if ($running) {
            Stop-Process -Name $proc -Force -ErrorAction SilentlyContinue
            Write-Status "Proceso $proc detenido" "OK"
        }
    }
    
    Start-Sleep -Seconds 2
}

function Start-CppServer {
    Write-Status "Iniciando servidor C++ (Cerebro)..." "STEP"
    
    $serverPath = Join-Path $Config.InstallDir "src\$($Config.ServerExe)"
    $logDir = Join-Path $Config.InstallDir "logs"
    $logFile = Join-Path $logDir "server_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
    
    # Crear directorio de logs si no existe
    if (-not (Test-Path $logDir)) {
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    }
    
    # Verificar que existe el ejecutable
    if (-not (Test-Path $serverPath)) {
        throw "No se encontro $serverPath"
    }
    
    # Iniciar proceso
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = $serverPath
    $processInfo.Arguments = $Config.ServerArgs
    $processInfo.WorkingDirectory = Join-Path $Config.InstallDir "src"
    $processInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Minimized
    $processInfo.RedirectStandardOutput = $true
    $processInfo.RedirectStandardError = $true
    $processInfo.UseShellExecute = $false
    
    $process = [System.Diagnostics.Process]::Start($processInfo)
    
    # Guardar PID para referencia
    $process.Id | Out-File (Join-Path $Config.InstallDir "server.pid") -Force
    
    Write-Status "Servidor iniciado (PID: $($process.Id))" "OK"
    
    return $process
}

function Wait-ForServer {
    Write-Status "Esperando que el servidor este listo..." "STEP"
    
    $port = $Config.ServerPort
    $maxWait = $Config.MaxWaitSeconds
    
    for ($i = 1; $i -le $maxWait; $i++) {
        Write-Progress -Activity "Conectando al servidor" -Status "Intento $i de $maxWait" -PercentComplete (($i / $maxWait) * 100)
        
        if (Test-ServerReady -Port $port) {
            Write-Progress -Activity "Conectando al servidor" -Completed
            Write-Status "Servidor listo en puerto $port" "OK"
            return $true
        }
        
        Start-Sleep -Seconds 1
    }
    
    Write-Progress -Activity "Conectando al servidor" -Completed
    return $false
}

function Start-ElectronUI {
    param([switch]$KioskMode)
    
    Write-Status "Iniciando interfaz Totem..." "STEP"
    
    $totemPath = Join-Path $Config.InstallDir "totem"
    
    # Verificar node_modules
    $nodeModules = Join-Path $totemPath "node_modules"
    if (-not (Test-Path $nodeModules)) {
        Write-Status "Instalando dependencias npm..." "WARN"
        Push-Location $totemPath
        npm install --silent
        Pop-Location
    }
    
    # Construir argumentos
    $args = @("electron", ".")
    if ($KioskMode) {
        $args += "--kiosk"
    }
    $args += "--no-backend"
    
    # Iniciar Electron
    Push-Location $totemPath
    $electronProcess = Start-Process -FilePath "npx" -ArgumentList $args -PassThru -WindowStyle Normal
    Pop-Location
    
    Write-Status "Totem UI iniciado (PID: $($electronProcess.Id))" "OK"
    
    return $electronProcess
}

function Show-SystemInfo {
    $info = @"

==========================================
   SISTEMA INICIADO CORRECTAMENTE
==========================================

  Servidor C++:  http://localhost:$($Config.ServerPort)
  API Status:    http://localhost:$($Config.ServerPort)/api/sensor/status
  Logs:          $($Config.InstallDir)\logs\

==========================================
   INSTRUCCIONES
==========================================

  - Para cerrar: Cierra la ventana del Totem
  - El servidor se cerrara automaticamente
  - Presiona Ctrl+C para forzar cierre

==========================================

"@
    Write-Host $info -ForegroundColor Green
}

# ============================================
# MAIN
# ============================================
try {
    Clear-Host
    Show-Banner
    
    # Verificar instalacion
    Write-Status "Verificando instalacion..." "STEP"
    if (-not (Test-Path $Config.InstallDir)) {
        throw "Directorio de instalacion no encontrado: $($Config.InstallDir)"
    }
    Write-Status "Instalacion verificada" "OK"
    
    # Detener procesos existentes
    Stop-ExistingProcesses
    
    # Iniciar servidor C++
    $serverProcess = Start-CppServer
    
    # Esperar servidor
    $serverReady = Wait-ForServer
    if (-not $serverReady) {
        throw "Timeout esperando al servidor. Revisa los logs."
    }
    
    # Iniciar UI (si no es modo NoUI)
    if (-not $NoUI) {
        $uiProcess = Start-ElectronUI -KioskMode:$Kiosk
    }
    
    # Mostrar informacion
    Show-SystemInfo
    
    # Esperar a que se cierre Electron
    if (-not $NoUI) {
        Write-Status "Sistema corriendo. Esperando cierre del Totem..." "INFO"
        $uiProcess.WaitForExit()
        
        Write-Status "Totem cerrado. Deteniendo servidor..." "INFO"
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    }
    else {
        Write-Status "Modo sin UI. Presiona Ctrl+C para detener." "INFO"
        while ($true) {
            Start-Sleep -Seconds 5
            if (-not (Test-ServerReady)) {
                Write-Status "Servidor detenido" "WARN"
                break
            }
        }
    }
    
    Write-Status "Sistema detenido correctamente" "OK"
}
catch {
    Write-Status $_.Exception.Message "ERROR"
    Write-Status "Stack: $($_.ScriptStackTrace)" "ERROR"
    
    # Limpiar procesos en caso de error
    Stop-Process -Name "Digitador" -Force -ErrorAction SilentlyContinue
    Stop-Process -Name "electron" -Force -ErrorAction SilentlyContinue
    
    exit 1
}