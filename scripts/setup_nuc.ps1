# =============================================================================
# setup_nuc.ps1 — Corre en el NUC (maquina de produccion)
# Instala Digitador y lo registra como tarea de inicio automatico en Windows
# =============================================================================
# USO: Ejecutar como Administrador desde la carpeta donde se extrajeron los archivos
#      (la misma carpeta donde esta digitador.exe)
# =============================================================================

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"
$INSTALL_DIR = "C:\Digitador"
$TASK_NAME   = "DigitadorTotem"
$SRC         = $PSScriptRoot  # carpeta del zip descomprimido

function Log-OK   { param($m) Write-Host "  [OK] $m" -ForegroundColor Green }
function Log-INFO { param($m) Write-Host "  [..] $m" -ForegroundColor Cyan }
function Log-WARN { param($m) Write-Host "  [!!] $m" -ForegroundColor Yellow }
function Log-FAIL { param($m) Write-Host "  [XX] $m" -ForegroundColor Red; exit 1 }

Write-Host "`n==========================================" -ForegroundColor Magenta
Write-Host "  DIGITADOR — Setup NUC" -ForegroundColor Magenta
Write-Host "==========================================`n" -ForegroundColor Magenta

# =============================================================================
# FASE 1: Verificar Node.js
# =============================================================================
Log-INFO "Verificando Node.js..."
if (-not (Get-Command "node" -ErrorAction SilentlyContinue)) {
    $nodeInstaller = Get-ChildItem $SRC -Filter "node-*.msi" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($nodeInstaller) {
        Log-INFO "Instalando Node.js desde $($nodeInstaller.Name)..."
        Start-Process msiexec.exe -ArgumentList "/i `"$($nodeInstaller.FullName)`" /quiet /norestart" -Wait
        # Refrescar PATH
        $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "User")
        Log-OK "Node.js instalado"
    } else {
        Log-FAIL "Node.js no encontrado y no hay instalador .msi en esta carpeta. Descargarlo de https://nodejs.org y copiarlo aqui antes de ejecutar."
    }
} else {
    Log-OK "Node.js $(node --version)"
}

# =============================================================================
# FASE 2: Verificar Python y dependencias
# =============================================================================
Log-INFO "Verificando Python..."
if (-not (Get-Command "python" -ErrorAction SilentlyContinue)) {
    $pyInstaller = Get-ChildItem $SRC -Filter "python-*.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($pyInstaller) {
        Log-INFO "Instalando Python desde $($pyInstaller.Name)..."
        Start-Process $pyInstaller.FullName -ArgumentList "/quiet InstallAllUsers=1 PrependPath=1" -Wait
        $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "User")
        Log-OK "Python instalado"
    } else {
        Log-WARN "Python no encontrado y no hay instalador en esta carpeta. SubirExcel.py no funcionara."
    }
}

if (Get-Command "pip" -ErrorAction SilentlyContinue) {
    Log-INFO "Instalando dependencias Python (pandas, openpyxl)..."
    pip install pandas openpyxl --quiet
    Log-OK "Dependencias Python instaladas"
}

# =============================================================================
# FASE 3: Instalar archivos en C:\Digitador\
# =============================================================================
Log-INFO "Instalando en $INSTALL_DIR..."
if (-not (Test-Path $INSTALL_DIR)) {
    New-Item -ItemType Directory -Path $INSTALL_DIR | Out-Null
}

Copy-Item "$SRC\*" $INSTALL_DIR -Recurse -Force
Log-OK "Archivos copiados a $INSTALL_DIR"

# Copiar libzkfp.dll al lado del exe para que Windows la encuentre sin PATH extra
$dll = "$INSTALL_DIR\libs\libzkfp.dll"
if (Test-Path $dll) {
    Copy-Item $dll "$INSTALL_DIR\libzkfp.dll" -Force
    Log-OK "libzkfp.dll colocada junto al ejecutable"
} else {
    Log-WARN "libzkfp.dll no encontrada en libs\ — el sensor biometrico no funcionara"
}

# Verificar que el exe existe
$exePath = "$INSTALL_DIR\digitador.exe"
if (-not (Test-Path $exePath)) {
    Log-FAIL "digitador.exe no encontrado en $INSTALL_DIR. Verificar que el build se completo correctamente."
}

# =============================================================================
# FASE 4: Registrar tarea en el Task Scheduler (inicio automatico al login)
# =============================================================================
Log-INFO "Registrando tarea de inicio automatico '$TASK_NAME'..."

# Eliminar tarea anterior si existe
if (Get-ScheduledTask -TaskName $TASK_NAME -ErrorAction SilentlyContinue) {
    Unregister-ScheduledTask -TaskName $TASK_NAME -Confirm:$false
    Log-INFO "Tarea anterior eliminada"
}

$action = New-ScheduledTaskAction `
    -Execute $exePath `
    -Argument "--totem" `
    -WorkingDirectory $INSTALL_DIR

# Dispara al iniciar sesion cualquier usuario
$trigger = New-ScheduledTaskTrigger -AtLogOn

# Corre con los permisos del usuario que inicia sesion (necesita UI/escritorio)
$principal = New-ScheduledTaskPrincipal `
    -GroupId "BUILTIN\Users" `
    -RunLevel Limited

$settings = New-ScheduledTaskSettingsSet `
    -ExecutionTimeLimit ([TimeSpan]::Zero) `
    -RestartCount 5 `
    -RestartInterval (New-TimeSpan -Minutes 1) `
    -StopIfGoingOnBatteries $false `
    -DisallowStartIfOnBatteries $false `
    -MultipleInstances IgnoreNew

Register-ScheduledTask `
    -TaskName $TASK_NAME `
    -Action $action `
    -Trigger $trigger `
    -Principal $principal `
    -Settings $settings `
    -Description "Inicia el totem Digitador automaticamente al encender el NUC" `
    -Force | Out-Null

Log-OK "Tarea '$TASK_NAME' registrada (se ejecuta al iniciar sesion)"

# =============================================================================
# FASE 5: Configurar inicio de sesion automatico (opcional, modo kiosk)
# =============================================================================
Log-INFO "Configurando inicio de sesion automatico..."
$autoLoginUser = Read-Host "  Nombre de usuario de Windows para auto-login (dejar vacio para omitir)"

if ($autoLoginUser -ne "") {
    $autoLoginPass = Read-Host "  Contrasena de $autoLoginUser (se almacena en el registro)" -AsSecureString
    $plainPass = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
        [Runtime.InteropServices.Marshal]::SecureStringToBSTR($autoLoginPass)
    )

    $regPath = "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon"
    Set-ItemProperty $regPath "AutoAdminLogon"  "1"
    Set-ItemProperty $regPath "DefaultUserName"  $autoLoginUser
    Set-ItemProperty $regPath "DefaultPassword"  $plainPass
    Log-OK "Auto-login configurado para '$autoLoginUser'"
    Log-WARN "La contrasena queda en el registro de Windows. Solo usar en equipos fisicamente seguros."
} else {
    Log-INFO "Auto-login omitido. Configurarlo manualmente con: netplwiz"
}

# =============================================================================
# RESUMEN FINAL
# =============================================================================
Write-Host "`n==========================================" -ForegroundColor Magenta
Write-Host "  INSTALACION COMPLETADA" -ForegroundColor Magenta
Write-Host "==========================================`n" -ForegroundColor Magenta
Write-Host "  Instalado en : $INSTALL_DIR" -ForegroundColor Green
Write-Host "  Tarea        : $TASK_NAME (se activa al iniciar sesion)" -ForegroundColor Green
Write-Host ""
Write-Host "  PARA VERIFICAR:" -ForegroundColor Yellow
Write-Host "    1. Reiniciar el NUC"
Write-Host "    2. El totem debe abrirse solo al llegar al escritorio"
Write-Host "    3. Si falla, revisar el log en:"
Write-Host "       Visor de eventos > Registros de Windows > Aplicacion"
Write-Host ""
Write-Host "  PARA DESINSTALAR:" -ForegroundColor Yellow
Write-Host "    Unregister-ScheduledTask -TaskName $TASK_NAME -Confirm:`$false"
Write-Host ""
