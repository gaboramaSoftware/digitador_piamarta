@echo off
cls
echo ==========================================
echo   COMPILAR Y EJECUTAR SERVIDOR WEB
echo   Puerto: 18080
echo ==========================================
echo.

:: 1. CONFIGURACIÓN
set EXECUTABLE=Digitador.exe

:: Rutas
set INCLUDE_DIRS=-I. -Ih -IBD -Ilibs/include
set LIB_DIRS=-Llibs/x64lib
set LIBS=-lzkfp -lws2_32 -lmswsock

:: 2. COMPILAR SQLITE CON GCC (C)
echo [1/2] Compilando SQLite (C)...
gcc -c BD/sqlite3.c -o BD/sqlite3.o
if %errorlevel% neq 0 (
    echo [X] Error compilando SQLite.
    pause
    exit /b %errorlevel%
)
echo [OK] SQLite compilado correctamente.
echo.

:: 3. COMPILAR PROYECTO C++ Y LINKEAR SQLITE
set CPP_SOURCES=main.cpp cpp/DB_Backend.cpp cpp/Sensor.cpp cpp/HardwareManager.cpp cpp/TemplateManager.cpp cpp/HeadlessManager.cpp cpp/CrowServer.cpp
set CPP_FLAGS=-std=c++17 -pthread -static-libgcc -static-libstdc++ -DASIO_STANDALONE -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601

echo [2/2] Compilando proyecto C++...
g++ %CPP_SOURCES% BD/sqlite3.o %INCLUDE_DIRS% %LIB_DIRS% %LIBS% %CPP_FLAGS% -o %EXECUTABLE%

if %errorlevel% neq 0 (
    echo.
    echo [X] ERROR EN LA COMPILACION.
    pause
    exit /b %errorlevel%
)
echo [OK] Proyecto compilado correctamente.
echo.

:: 4. EJECUTAR SERVIDOR WEB
echo ==========================================
echo   INICIANDO SERVIDOR WEB CROW
echo ==========================================
echo.
echo [+] Servidor web iniciando en puerto 18080
echo [+] API endpoints disponibles:
echo     - http://localhost:18080/api/dashboard
echo     - http://localhost:18080/api/recent (incluye nombres completos)
echo.
echo [!] Para detener el servidor: Ctrl+C
echo.
echo ==========================================
echo.

%EXECUTABLE% --webserver

echo.
echo [!] Servidor detenido.
pause
