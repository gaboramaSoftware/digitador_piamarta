@echo off
TITLE DIGITADOR - ESTADO DEL SISTEMA
COLOR 0B

SET "SERVER_PORT=18080"

echo.
echo ==========================================
echo    ESTADO DEL SISTEMA DIGITADOR
echo ==========================================
echo.

:: Verificar proceso del servidor
echo [PROCESOS]
echo.
tasklist /FI "IMAGENAME eq Digitador.exe" 2>nul | find /I "Digitador.exe" >nul
IF %errorlevel% EQU 0 (
    echo   Servidor C++ (Digitador.exe): [CORRIENDO]
) ELSE (
    echo   Servidor C++ (Digitador.exe): [DETENIDO]
)

tasklist /FI "IMAGENAME eq electron.exe" 2>nul | find /I "electron.exe" >nul
IF %errorlevel% EQU 0 (
    echo   Totem UI (electron.exe):      [CORRIENDO]
) ELSE (
    echo   Totem UI (electron.exe):      [DETENIDO]
)

echo.
echo [CONECTIVIDAD]
echo.

:: Verificar API
curl -s -o nul -w "%%{http_code}" "http://localhost:%SERVER_PORT%/api/sensor/status" 2>nul | findstr "200" >nul
IF %errorlevel% EQU 0 (
    echo   API Server ^(puerto %SERVER_PORT%^):     [RESPONDIENDO]
    echo.
    echo [ESTADO DEL SENSOR]
    echo.
    curl -s "http://localhost:%SERVER_PORT%/api/sensor/status" 2>nul
    echo.
) ELSE (
    echo   API Server ^(puerto %SERVER_PORT%^):     [NO RESPONDE]
)

echo.
echo ==========================================
echo.
pause