@echo off
cls
echo ==========================================
echo    COMPILANDO PROYECTO DIGITADOR (SRC)
echo ==========================================

:: 1. CONFIGURACIÓN
set EXECUTABLE=Digitador.exe

:: Rutas
set INCLUDE_DIRS=-I. -Ih -IBD -Ilibs/include
set LIB_DIRS=-Llibs/x64lib
set LIBS=-lzkfp -lws2_32 -lmswsock

:: 2. PASO CRÍTICO: COMPILAR SQLITE CON GCC (C)
:: SQLite es código C, no C++. Si lo compilas con g++, explota.
:: Lo compilamos primero a un archivo objeto (.o)
echo [1/2] Compilando nucleo SQLite (C)...
gcc -c BD/sqlite3.c -o BD/sqlite3.o
if %errorlevel% neq 0 (
    echo [X] Error compilando SQLite.
    pause
    exit /b %errorlevel%
)

:: 3. COMPILAR TU PROYECTO CON G++ (C++) Y LINKEAR SQLITE
:: Aqui listamos tus archivos .cpp
set CPP_SOURCES=main.cpp cpp/DB_Backend.cpp cpp/Sensor.cpp cpp/HardwareManager.cpp cpp/TemplateManager.cpp cpp/HeadlessManager.cpp cpp/CrowServer.cpp cpp/Endpoints.cpp

:: Banderas de C++ (Tus flags originales)
set CPP_FLAGS=-std=c++17 -pthread -static-libgcc -static-libstdc++ -DASIO_STANDALONE -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601

echo [2/2] Compilando logica de Negocio (C++)...
:: Nota como agregamos "BD/sqlite3.o" al comando final
g++ %CPP_SOURCES% BD/sqlite3.o %INCLUDE_DIRS% %LIB_DIRS% %LIBS% %CPP_FLAGS% -o %EXECUTABLE%

:: 4. VERIFICACIÓN FINAL
if %errorlevel% neq 0 (
    echo.
    echo [X] ERROR FATAL EN LA COMPILACION DEL PROYECTO.
    pause
    exit /b %errorlevel%
)

:: Limpieza opcional (borrar el objeto temporal de sqlite)
:: del BD\sqlite3.o 

echo.
echo [OK] Exito. Ejecutando programa...
echo ------------------------------------------
%EXECUTABLE%
pause