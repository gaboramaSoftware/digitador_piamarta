@echo off
TITLE LAUNCHER SISTEMA BIOMETRICO
COLOR 0A

echo ==========================================
echo    INICIANDO ECOSISTEMA DIGITADOR
echo ==========================================

:: 1. MATAR PROCESOS VIEJOS (Para evitar puertos bloqueados)
echo [1/3] Limpiando procesos anteriores...
taskkill /F /IM Digitador.exe >nul 2>&1
taskkill /F /IM electron.exe >nul 2>&1

:: 2. INICIAR CEREBRO C++
echo [2/3] Arrancando Cerebro C++ (Puerto 18080)...
:: AJUSTA ESTA RUTA A DONDE ESTÁ TU .EXE REAL (No el .bat, el .exe compilado)
:: Usualmente en src/Debug o src/Release
start "CEREBRO_C++" /D "C:\Digitador\src\" "Digitador.exe"

:: Esperamos 5 segundos para asegurar que el servidor C++ esté listo
echo    ...Esperando que el cerebro cargue base de datos...
timeout /t 5 /nobreak >nul

:: 3. INICIAR TOTEM (ELECTRON)
echo [3/3] Arrancando Interfaz Totem...
cd /d "C:\totem"
:: Iniciamos npm start en una ventana separada o directa
start "TOTEM_UI" npm start

echo.
echo ==========================================
echo    SISTEMA CORRIENDO
echo    - C++ Server: Ventana minimizada
echo    - Totem: Abriendo en pantalla completa
echo ==========================================
pause