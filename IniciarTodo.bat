@echo off
TITLE LAUNCHER SISTEMA BIOMETRICO
COLOR 0A

echo ==========================================
echo    INICIANDO ECOSISTEMA DIGITADOR
echo ==========================================

@echo off
TITLE LAUNCHER SISTEMA BIOMETRICO
COLOR 0A

echo ==========================================
echo    INICIANDO ECOSISTEMA DIGITADOR
echo ==========================================

:: 1. MATAR PROCESOS VIEJOS
echo [1/4] Limpiando procesos anteriores...
taskkill /F /IM Digitador.exe >nul 2>&1
taskkill /F /IM electron.exe >nul 2>&1
timeout /t 2 >nul

:: 2. INICIAR SERVIDOR C++ EN SEGUNDO PLANO
echo [2/4] Iniciando Cerebro C++ (servidor web)...
cd /d "C:\Digitador\src"
start "CEREBRO_CPP" /MIN Digitador.exe --webserver

:: 3. ESPERAR A QUE EL SERVIDOR ESTÉ LISTO
echo [3/4] Esperando que el servidor inicie...
:wait_loop
timeout /t 1 >nul
curl -s http://localhost:18080/api/sensor/status >nul 2>&1
if %errorlevel% neq 0 (
    echo       Esperando servidor...
    goto wait_loop
)
echo       Servidor listo!

:: 4. INICIAR ELECTRON (sin que inicie el C++)
echo [4/4] Iniciando interfaz Totem...
cd /d "C:\Digitador\totem"
start "TOTEM_UI" npx electron . --no-backend

echo.
echo ==========================================
echo    SISTEMA INICIADO CORRECTAMENTE
echo ==========================================
echo.
echo [*] Cerebro C++: http://localhost:18080
echo [*] Para cerrar: Cierra la ventana del Totem
echo.
pause