@echo off
TITLE DIGITADOR - INSTALADOR
COLOR 0E
SETLOCAL EnableDelayedExpansion

echo.
echo ==========================================
echo    INSTALADOR DIGITADOR PIAMARTA
echo ==========================================
echo.

SET "INSTALL_DIR=C:\Digitador"

:: ============================================
:: VERIFICAR PRIVILEGIOS DE ADMINISTRADOR
:: ============================================
net session >nul 2>&1
IF %errorlevel% NEQ 0 (
    echo [WARN] Este script requiere privilegios de administrador
    echo [WARN] para algunas funciones. Continuando sin ellos...
    echo.
)

:: ============================================
:: PASO 1: VERIFICAR PREREQUISITOS
:: ============================================
echo [1/6] Verificando prerequisitos...

:: Verificar Node.js
where node >nul 2>&1
IF %errorlevel% NEQ 0 (
    echo       [ERROR] Node.js no encontrado!
    echo       [ERROR] Descarga e instala Node.js desde: https://nodejs.org/
    pause
    exit /b 1
)

FOR /F "tokens=*" %%i IN ('node --version') DO SET NODE_VERSION=%%i
echo       [OK] Node.js encontrado: %NODE_VERSION%

:: Verificar npm
where npm >nul 2>&1
IF %errorlevel% NEQ 0 (
    echo       [ERROR] npm no encontrado!
    pause
    exit /b 1
)

FOR /F "tokens=*" %%i IN ('npm --version') DO SET NPM_VERSION=%%i
echo       [OK] npm encontrado: v%NPM_VERSION%

:: Verificar curl
where curl >nul 2>&1
IF %errorlevel% NEQ 0 (
    echo       [WARN] curl no encontrado - algunas funciones pueden fallar
) ELSE (
    echo       [OK] curl encontrado
)

:: ============================================
:: PASO 2: CREAR ESTRUCTURA DE DIRECTORIOS
:: ============================================
echo [2/6] Creando estructura de directorios...

IF NOT EXIST "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
    echo       [OK] Creado: %INSTALL_DIR%
) ELSE (
    echo       [--] Ya existe: %INSTALL_DIR%
)

IF NOT EXIST "%INSTALL_DIR%\logs" (
    mkdir "%INSTALL_DIR%\logs"
    echo       [OK] Creado: %INSTALL_DIR%\logs
)

IF NOT EXIST "%INSTALL_DIR%\BD" (
    mkdir "%INSTALL_DIR%\BD"
    echo       [OK] Creado: %INSTALL_DIR%\BD
)

:: ============================================
:: PASO 3: VERIFICAR ARCHIVOS PRINCIPALES
:: ============================================
echo [3/6] Verificando archivos principales...

SET "MISSING_FILES=0"

IF NOT EXIST "%INSTALL_DIR%\src\Digitador.exe" (
    echo       [FALTA] src\Digitador.exe
    SET "MISSING_FILES=1"
) ELSE (
    echo       [OK] src\Digitador.exe
)

IF NOT EXIST "%INSTALL_DIR%\totem\package.json" (
    echo       [FALTA] totem\package.json
    SET "MISSING_FILES=1"
) ELSE (
    echo       [OK] totem\package.json
)

IF NOT EXIST "%INSTALL_DIR%\bin\libzkfp.dll" (
    echo       [FALTA] bin\libzkfp.dll (SDK ZKTeco)
    SET "MISSING_FILES=1"
) ELSE (
    echo       [OK] bin\libzkfp.dll
)

IF "%MISSING_FILES%"=="1" (
    echo.
    echo [WARN] Faltan archivos. Asegurate de copiar todos los archivos del proyecto.
    echo.
)

:: ============================================
:: PASO 4: INSTALAR DEPENDENCIAS TOTEM
:: ============================================
echo [4/6] Instalando dependencias del Totem...

IF EXIST "%INSTALL_DIR%\totem\package.json" (
    cd /d "%INSTALL_DIR%\totem"
    
    IF NOT EXIST "node_modules" (
        echo       Ejecutando npm install...
        call npm install
        IF %errorlevel% EQU 0 (
            echo       [OK] Dependencias instaladas
        ) ELSE (
            echo       [ERROR] Fallo npm install
        )
    ) ELSE (
        echo       [--] node_modules ya existe
    )
) ELSE (
    echo       [SKIP] No hay package.json en totem
)

:: ============================================
:: PASO 5: CREAR ACCESO DIRECTO EN ESCRITORIO
:: ============================================
echo [5/6] Creando acceso directo...

SET "DESKTOP=%USERPROFILE%\Desktop"
SET "SHORTCUT=%DESKTOP%\Digitador Piamarta.lnk"

:: Crear acceso directo usando PowerShell
powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%SHORTCUT%'); $Shortcut.TargetPath = '%INSTALL_DIR%\IniciarDigitador.bat'; $Shortcut.WorkingDirectory = '%INSTALL_DIR%'; $Shortcut.Description = 'Sistema Biometrico Digitador Piamarta'; $Shortcut.Save()" 2>nul

IF EXIST "%SHORTCUT%" (
    echo       [OK] Acceso directo creado en el escritorio
) ELSE (
    echo       [WARN] No se pudo crear acceso directo
)

:: ============================================
:: PASO 6: CONFIGURAR INICIO AUTOMATICO (OPCIONAL)
:: ============================================
echo [6/6] Configuracion de inicio automatico...

SET /P AUTO_START="Deseas que el sistema inicie automaticamente con Windows? (s/n): "

IF /I "%AUTO_START%"=="s" (
    SET "STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
    copy "%INSTALL_DIR%\IniciarDigitador.bat" "%STARTUP%\IniciarDigitador.bat" >nul 2>&1
    IF %errorlevel% EQU 0 (
        echo       [OK] Configurado inicio automatico
    ) ELSE (
        echo       [ERROR] No se pudo configurar inicio automatico
    )
) ELSE (
    echo       [--] Inicio automatico omitido
)

:: ============================================
:: INSTALACION COMPLETA
:: ============================================
echo.
echo ==========================================
echo    INSTALACION COMPLETADA
echo ==========================================
echo.
echo  Directorio: %INSTALL_DIR%
echo.
echo  Archivos disponibles:
echo    - IniciarDigitador.bat  : Iniciar el sistema
echo    - DetenerDigitador.bat  : Detener el sistema
echo    - EstadoSistema.bat     : Ver estado actual
echo.
echo  Acceso directo creado en el escritorio.
echo.
echo ==========================================
echo.

pause