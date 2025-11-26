@echo off
cls
echo ==========================================
echo   COMPILANDO PROYECTO DIGITADOR
echo ==========================================

cd /d "%~dp0"

rem 1) CONFIGURACION
set "EXECUTABLE=Digitador.exe"
set "COMPILER=g++"

rem Flags:
set "CFLAGS=-x c -c"
set "CPPFLAGS=-std=c++17 -Wall -Wextra -Ih -IBD -Ilibs\include"

rem Directorio de la lib ZKFP (VERSION 64 BITS)
set "ZKFP_LIB=libs\x64lib\libzkfp.lib"

echo.
echo [INFO] Verificando libreria ZKFP...
if not exist "%ZKFP_LIB%" (
    echo [X] No se encontro "%ZKFP_LIB%"
    echo     Asegurate de que exista ese archivo y que sea la version de 64 bits.
    goto build_error
) else (
    echo [INFO] Usando libreria ZKFP: %ZKFP_LIB%
)

echo.
echo [1/3] Compilando motor SQLite (C) ...
%COMPILER% %CFLAGS% BD\sqlite3.c -o BD\sqlite3.o
if errorlevel 1 goto build_error

echo.
echo [2/3] Compilando fuentes C++ ...
set "SOURCES=main.cpp cpp\*.cpp"

rem OJO: ahora NO usamos -lzkfp, sino la ruta completa al .lib
%COMPILER% %CPPFLAGS% %SOURCES% BD\sqlite3.o "%ZKFP_LIB%" -o "%EXECUTABLE%"
if errorlevel 1 goto build_error

echo.
echo [3/3] Compilacion exitosa.
echo ------------------------------------------
echo Ejecutando %EXECUTABLE% ...
echo ------------------------------------------
%EXECUTABLE%
pause
goto :eof

:build_error
echo.
echo [X] ERROR FATAL EN LA COMPILACION.
echo     Revisa rutas, includes o funciones nuevas sin implementar.
pause
exit /b 1
