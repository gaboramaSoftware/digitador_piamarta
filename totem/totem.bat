@echo off
title TOTEM - Kiosko
cls
echo ==========================================
echo   INICIANDO TOTEM EN MODO KIOSKO
echo ==========================================
echo.

cd /d "%~dp0"

:: Verificar dependencias
if not exist "node_modules" (
    echo [*] Instalando dependencias...
    npm install
    echo.
)

echo [!] Iniciando Electron...
echo.

:: Iniciar en modo kiosk
electron . --kiosk

:: Si falla, mostrar mensaje
if errorlevel 1 (
    echo.
    echo [X] Error al iniciar Electron
    echo [*] Intentando con npm...
    npm start
)

echo.
echo ==========================================
echo [!] Aplicacion cerrada
echo ==========================================
timeout /t 5