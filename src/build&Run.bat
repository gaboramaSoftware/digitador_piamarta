@echo off
setlocal ENABLEDELAYEDEXPANSION
title Build & Run (g++ MinGW)

REM --- Ubicaciones base (relativas al .bat) ---
set "ROOT=%~dp0"
set "SRC_DIR=%ROOT%cpp"
set "INC1=%ROOT%h"
set "INC2=%ROOT%libs\include"
set "LIBDIR=%ROOT%libs\lib"
set "OUT_EXE=%ROOT%app.exe"

REM --- Asegurar g++ en PATH (MSYS2 MinGW64 por defecto) ---
where g++ >NUL 2>&1
if errorlevel 1 (
  set "MINGW_BIN=C:\msys64\mingw64\bin"
  if exist "%MINGW_BIN%\g++.exe" (
    set "PATH=%MINGW_BIN%;%PATH%"
  ) else (
    echo [ERROR] No se encontro g++. Instala MSYS2 y el toolchain MinGW64, o agrega g++ al PATH.
    pause
    exit /b 1
  )
)

REM --- Juntar los .cpp ---
if not exist "%SRC_DIR%" (
  echo [ERROR] No existe el directorio de fuentes: %SRC_DIR%
  pause
  exit /b 1
)

set "SRC_FILES="
for %%F in ("%SRC_DIR%\*.cpp") do (
  set "SRC_FILES=!SRC_FILES! "%%F""
)

if "!SRC_FILES!"=="" (
  echo [ERROR] No se encontraron .cpp en %SRC_DIR%
  pause
  exit /b 1
)

REM --- Compilar y enlazar ---
echo ================= COMPILANDO =================
echo g++ -std=c++20 -Wall -Wextra -O2 ^
 %SRC_FILES% ^
 -I"%INC1%" -I"%INC2%" -L"%LIBDIR%" -lsqlite3 -o "%OUT_EXE%"
echo.

g++ -std=c++20 -Wall -Wextra -O2 ^
 %SRC_FILES% ^
 -I"%INC1%" -I"%INC2%" -L"%LIBDIR%" -lsqlite3 -o "%OUT_EXE%"

if errorlevel 1 (
  echo.
  echo [FAIL] La compilacion fallo. Revisa los mensajes de arriba.
  pause
  exit /b 1
)

REM --- Copiar sqlite3.dll junto al exe si existe en MinGW64 ---
if exist "C:\msys64\mingw64\bin\sqlite3.dll" (
  copy /Y "C:\msys64\mingw64\bin\sqlite3.dll" "%ROOT%" >NUL
)

echo.
echo [OK] Compilacion exitosa: "%OUT_EXE%"
echo =================  EJECUTANDO  =================
echo.

"%OUT_EXE%"
set "RET=%ERRORLEVEL%"
echo.
echo [INFO] app.exe retorno codigo %RET%
pause
exit /b %RET%
