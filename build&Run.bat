@echo off
cls
echo ==========================================
echo    COMPILANDO PROYECTO DIGITADOR (CORREGIDO)
echo ==========================================

:: 1. CONFIGURACIÓN
set EXECUTABLE=Digitador.exe
set COMPILER=g++

:: 2. ARCHIVOS FUENTE (.cpp y .c)
:: Aquí listamos TODOS los archivos que se ven en tu 'tree'
:: main.cpp             -> Raíz
:: BD/sqlite3.c         -> Motor SQLite
:: cpp/*.cpp            -> Toda tu lógica (DB_Backend, Sensor, HardwareManager, etc.)
set SOURCES=main.cpp BD/sqlite3.c cpp/DB_Backend.cpp cpp/Sensor.cpp cpp/HardwareManager.cpp cpp/Business_Logic.cpp cpp/Sync_Service.cpp

:: 3. RUTAS DE CABECERAS (.h / .hpp)
:: -I.              -> Carpeta actual
:: -Ih              -> Tu carpeta de headers propios (DB_models.hpp, Sensor.h, etc.)
:: -IBD             -> Para encontrar sqlite3.h
:: -Ilibs/include   -> Para encontrar libzkfp.h y los headers del SDK
set INCLUDES=-I. -Ih -IBD -Ilibs/include

:: 4. LIBRERÍAS EXTERNAS
:: -Llibs/x64lib    -> Importante: Apuntar a la subcarpeta x64 (o x86lib si tu g++ es de 32 bits)
:: -lzkfp           -> Enlaza con libzkfp.lib
set LIBS=-Llibs/x64lib -lzkfp

:: 5. BANDERAS
set FLAGS=-std=c++17 -pthread -static-libgcc -static-libstdc++

:: 6. COMANDO
echo Compilando...
%COMPILER% %SOURCES% %INCLUDES% %LIBS% %FLAGS% -o %EXECUTABLE%

:: 7. VERIFICACIÓN
if %errorlevel% neq 0 (
    echo.
    echo [X] ERROR FATAL EN LA COMPILACION.
    echo     Revisa si falta algun archivo o ruta.
    pause
    exit /b %errorlevel%
)

echo.
echo [OK] Exito. Ejecutando programa...
echo ------------------------------------------
%EXECUTABLE%
pause