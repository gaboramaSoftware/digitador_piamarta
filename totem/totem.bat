@echo off
:: Si sigue fallando, cambia la linea de arriba por "@echo on" para ver el error exacto
title TOTEM - UI
cls
echo ==========================================
echo   INICIANDO INTERFAZ TOTEM (ELECTRON)
echo ==========================================
echo.

:: 1. Ubicarse en la carpeta del script
cd /d "%~dp0"

:: 2. Verificacion rapida (AQUI FALTABA EL ESPACIO)
if not exist "package.json" (
    echo.
    echo [X] ERROR CRITICO: No se encuentra package.json.
    echo     Asegurate de poner este archivo .bat en la misma carpeta que el proyecto Electron.
    pause
    exit /b
)

:: 3. Ejecutar Electron
echo [!] Conectando con el nucleo de Electron...
echo.

:: Usamos 'call' para que si npm falla, no mate el script inmediatamente
call npm start

:: 4. Cierre
echo.
echo ==========================================
echo [!] La sesion del Totem ha finalizado.
echo ==========================================
pause
exit