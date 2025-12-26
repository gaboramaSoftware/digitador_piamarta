# Start Digitador System
Write-Host "Starting Digitador System..." -ForegroundColor Green

# 1. Start Brain (C++ Backend)
Write-Host "1. Starting Brain (Backend)..."
$brainPath = "c:\Digitador\src\Digitador.exe"
if (Test-Path $brainPath) {
    Start-Process -FilePath $brainPath -ArgumentList "--webserver" -WorkingDirectory "c:\Digitador\src"
    Write-Host "   El cerebro del sistema se inicio correctamente." -ForegroundColor Cyan
} else {
    Write-Host "   ERROR: El cerebro del sistema no se encontro en: $brainPath" -ForegroundColor Red
    Write-Host "   Por favor, construye el proyecto primero."
    exit 1
}

# Wait for Brain to initialize
Write-Host "   Waiting 5 seconds for Brain to initialize..."
Start-Sleep -Seconds 5

# 2. Start Totem (Electron App)
Write-Host "2. Inicializando Totem..."
$totemPath = "C:\Digitador\totem"
if (Test-Path $totemPath) {
    # Using Start-Process to run npm start in a new window/background
    Start-Process -FilePath "npm" -ArgumentList "start" -WorkingDirectory $totemPath
    Write-Host "   Totem iniciado correctamente." -ForegroundColor Cyan
} else {
    Write-Host "   ERROR: Sistema de Totem no encontrado en: $totemPath" -ForegroundColor Red
}

# 3. Start Desktop (Web Admin)
Write-Host "3. Inicializando Panel de Administración..."
Start-Process "http://localhost:18080"
Write-Host "   Panel de Administración abierto." -ForegroundColor Cyan

Write-Host "Todos componentes del sistema se inicializaron correctamente." -ForegroundColor Green
