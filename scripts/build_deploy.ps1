# =============================================================================
# build_deploy.ps1 — Corre en la MAQUINA DE DESARROLLO
# Compila el proyecto y arma el paquete listo para instalar en el NUC
# =============================================================================
# USO: .\scripts\build_deploy.ps1
# REQUISITOS PREVIOS (en tu maquina de desarrollo):
#   - Go 1.25+
#   - MinGW-w64 (GCC para CGO) — debe estar en PATH
#   - Node.js 22+
#   - Python 3.12+  +  pip install pandas openpyxl
# =============================================================================

$ErrorActionPreference = "Stop"
$ROOT = Split-Path $PSScriptRoot -Parent
$DIGITADOR = Join-Path $ROOT "Digitador"
$DIST = Join-Path $ROOT "dist"
$TOTEM = Join-Path $DIGITADOR "infra\totem"

# ——— Colores para logs ———
function Log-OK   { param($m) Write-Host "  [OK] $m" -ForegroundColor Green }
function Log-INFO { param($m) Write-Host "  [..] $m" -ForegroundColor Cyan }
function Log-WARN { param($m) Write-Host "  [!!] $m" -ForegroundColor Yellow }
function Log-FAIL { param($m) Write-Host "  [XX] $m" -ForegroundColor Red; exit 1 }

Write-Host "`n==========================================" -ForegroundColor Magenta
Write-Host "  DIGITADOR — Build & Deploy Script" -ForegroundColor Magenta
Write-Host "==========================================`n" -ForegroundColor Magenta

# =============================================================================
# FASE 1: Verificar prerequisitos en la maquina de desarrollo
# =============================================================================
Log-INFO "Verificando herramientas de build..."

if (-not (Get-Command "go" -ErrorAction SilentlyContinue)) {
    Log-FAIL "Go no encontrado. Instalar desde https://go.dev/dl/"
}
$goVersion = (go version) -replace "go version go",""
Log-OK "Go $goVersion"

if (-not (Get-Command "gcc" -ErrorAction SilentlyContinue)) {
    Log-FAIL "GCC no encontrado. Necesario para CGO (go-sqlite3). Instalar MinGW-w64."
}
Log-OK "GCC encontrado: $(gcc --version | Select-Object -First 1)"

if (-not (Get-Command "node" -ErrorAction SilentlyContinue)) {
    Log-FAIL "Node.js no encontrado."
}
Log-OK "Node.js $(node --version)"

if (-not (Get-Command "npm" -ErrorAction SilentlyContinue)) {
    Log-FAIL "npm no encontrado."
}

# Verificar que libzkfp.dll existe (viene del SDK de ZKTeco, NO esta en el repo)
$libzkfpDll = Join-Path $DIGITADOR "core\Hardware\Sensor\x64lib\libzkfp.dll"
if (-not (Test-Path $libzkfpDll)) {
    Log-WARN "libzkfp.dll no encontrada en core\Hardware\Sensor\x64lib\"
    Log-WARN "Buscar en el SDK de ZKTeco y copiarla ahi antes de compilar."
    Log-WARN "Sin ella el sensor biometrico no funcionara en el NUC."
    # No es fatal para el build, pero el .exe no correra en el NUC sin la DLL
}

# =============================================================================
# FASE 2: Preparar carpeta dist/
# =============================================================================
Log-INFO "Preparando carpeta dist/..."
if (Test-Path $DIST) {
    Remove-Item $DIST -Recurse -Force
}
New-Item -ItemType Directory -Path $DIST | Out-Null
New-Item -ItemType Directory -Path "$DIST\libs" | Out-Null
New-Item -ItemType Directory -Path "$DIST\drivers" | Out-Null
Log-OK "Carpeta dist/ creada en $DIST"

# =============================================================================
# FASE 3: Compilar binario Go con CGO
# =============================================================================
Log-INFO "Compilando digitador.exe (CGO habilitado)..."
Set-Location $DIGITADOR

$env:CGO_ENABLED = "1"
$env:GOOS = "windows"
$env:GOARCH = "amd64"

# go-sqlite3 necesita gcc; libzkfp se linka via el pragma en SensorAdapter.go
go build -v -o "$DIST\digitador.exe" .\cmd\main.go
if ($LASTEXITCODE -ne 0) {
    Log-FAIL "Fallo el build de Go. Revisar errores arriba."
}
Log-OK "digitador.exe compilado correctamente"

# =============================================================================
# FASE 4: Copiar DLLs de runtime nativo (libzkfp)
# =============================================================================
Log-INFO "Copiando DLLs de hardware..."

if (Test-Path $libzkfpDll) {
    Copy-Item $libzkfpDll "$DIST\libs\libzkfp.dll"
    Log-OK "libzkfp.dll copiada"
} else {
    Log-WARN "libzkfp.dll NO copiada — agregarla manualmente a dist\libs\ antes de instalar en el NUC"
}

# =============================================================================
# FASE 5: Pre-instalar node_modules del totem (Electron)
# =============================================================================
Log-INFO "Instalando dependencias de Electron en el totem..."
Set-Location $TOTEM
npm install --prefer-offline
if ($LASTEXITCODE -ne 0) {
    Log-FAIL "Fallo npm install en el totem."
}
Log-OK "node_modules instalados"

# Copiar totem al dist (con node_modules incluidos)
Log-INFO "Copiando totem a dist/..."
$destTotem = "$DIST\totem"
Copy-Item $TOTEM $destTotem -Recurse -Force
Log-OK "Totem copiado a dist\totem\"

# =============================================================================
# FASE 6: Copiar base de datos inicial y script Python
# =============================================================================
Log-INFO "Copiando archivos de datos..."

Copy-Item (Join-Path $DIGITADOR "core\db\digitador.db") "$DIST\digitador.db"
Log-OK "digitador.db copiada"

Copy-Item (Join-Path $DIGITADOR "app\excel\SubirExcel.py") "$DIST\SubirExcel.py"
Log-OK "SubirExcel.py copiado"

# =============================================================================
# FASE 7: Copiar driver de impresora POS
# =============================================================================
Log-INFO "Copiando driver de impresora POS..."
$srcDriver = Join-Path $DIGITADOR "core\Hardware\Impresora\POS Printer Driver V11.3.0.3"
if (Test-Path $srcDriver) {
    Copy-Item $srcDriver "$DIST\drivers\POS_Driver" -Recurse -Force
    Log-OK "Driver POS copiado"
} else {
    Log-WARN "Driver POS no encontrado en $srcDriver"
}

# =============================================================================
# FASE 8: Generar requirements.txt de Python para el NUC
# =============================================================================
@"
pandas>=2.2.0
openpyxl>=3.1.0
"@ | Set-Content "$DIST\requirements.txt"
Log-OK "requirements.txt generado"

# =============================================================================
# FASE 9: Copiar el script de instalacion del NUC al dist
# =============================================================================
$setupScript = Join-Path $PSScriptRoot "setup_nuc.ps1"
if (Test-Path $setupScript) {
    Copy-Item $setupScript "$DIST\setup_nuc.ps1"
    Log-OK "setup_nuc.ps1 incluido en dist/"
}

# =============================================================================
# FASE 10: Comprimir para pendrive
# =============================================================================
Log-INFO "Comprimiendo dist/ para pendrive..."
$zipPath = Join-Path $ROOT "digitador_deploy.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path "$DIST\*" -DestinationPath $zipPath
Log-OK "Paquete creado: digitador_deploy.zip"

# =============================================================================
# RESUMEN
# =============================================================================
Write-Host "`n==========================================" -ForegroundColor Magenta
Write-Host "  BUILD COMPLETADO" -ForegroundColor Magenta
Write-Host "==========================================`n" -ForegroundColor Magenta
Write-Host "  Contenido del paquete:" -ForegroundColor White
Get-ChildItem $DIST | ForEach-Object { Write-Host "    - $($_.Name)" }
Write-Host "`n  ZIP listo en: $zipPath" -ForegroundColor Green
Write-Host "  Copiar digitador_deploy.zip al pendrive y en el NUC:"
Write-Host "    1. Extraer el zip"
Write-Host "    2. Poner los instaladores de Node.js y Python en la carpeta extraida"
Write-Host "    3. Ejecutar setup_nuc.ps1 como Administrador`n"
