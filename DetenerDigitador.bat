@echo off
TITLE DIGITADOR - DETENIENDO SISTEMA
COLOR 0C

echo.
echo ==========================================
echo    DETENIENDO SISTEMA DIGITADOR
echo ==========================================
echo.

echo [1/2] Cerrando interfaz Totem...
taskkill /F /IM electron.exe >nul 2>&1
IF %errorlevel% EQU 0 (
    echo       [OK] Electron cerrado
) ELSE (
    echo       [--] Electron no estaba corriendo
)

echo [2/2] Cerrando servidor C++...
taskkill /F /IM Digitador.exe >nul 2>&1
IF %errorlevel% EQU 0 (
    echo       [OK] Servidor cerrado
) ELSE (
    echo       [--] Servidor no estaba corriendo
)

echo.
echo ==========================================
echo    SISTEMA DETENIDO
echo ==========================================
echo.
timeout /t 3
exit /b 0