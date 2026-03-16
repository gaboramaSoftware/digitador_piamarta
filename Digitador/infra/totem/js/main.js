// C:\Digitador\totem\js\main.js
const { app, BrowserWindow, screen, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const http = require('http');

//importaciones
const { startCerebro } = require('./startup/StartCerebro');
const { waitForServer } = require('./startup/EsperaServidor');

// --- VARIABLES ---
let mainWindow;
let loginWindow = null;
let cerebroProcess = null;

const preloadPath = path.join(__dirname, 'preload.js');

//inicio de la app
app.whenReady().then(async () => {
    console.log("==========================================");
    console.log("[MAIN] Iniciando Totem Digitador...");
    console.log("==========================================");

    // 1. Iniciar el backend C++
    startCerebro();

    // 2. Esperar a que esté listo
    const serverReady = await waitForServer();

    if (!serverReady) {
        console.error("[MAIN] ADVERTENCIA: El servidor no está disponible");
        console.error("[MAIN] El totem iniciará pero el sensor puede no funcionar");
    }

    // 3. Crear ventana principal
    createMainWindow();

    // --- IPC HANDLERS ---

    // Abrir ventana de autenticación
    ipcMain.on('app:open-auth', () => {
        createAuthWindow();
    });

    // Manejar login
    ipcMain.handle('auth:login', async (event, args) => {
        const { rut, password } = args || {};

        // Credenciales de administrador (sacarlas de Models.go)
        if (rut === '11111111' && password === 'admin123') {
            if (loginWindow) loginWindow.close();
            return { success: true, role: 'admin' };
        }

        return { success: false, error: 'Credenciales inválidas' };
    });

    // Cerrar aplicación
    ipcMain.on('app:close', () => {
        if (mainWindow) mainWindow.close();
    });

    // Minimizar aplicación
    ipcMain.on('app:minimize', () => {
        if (mainWindow) mainWindow.minimize();
    });

    // Recargar página
    ipcMain.on('app:reload', () => {
        if (mainWindow) mainWindow.reload();
    });
});

app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createMainWindow();
    }
});

app.on('window-all-closed', () => {
    // Matar proceso C++ al cerrar
    if (cerebroProcess && !cerebroProcess.killed) {
        console.log("[MAIN] Cerrando Cerebro C++...");
        cerebroProcess.kill();
    }

    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('will-quit', () => {
    // Asegurar que C++ se cierre
    if (cerebroProcess && !cerebroProcess.killed) {
        cerebroProcess.kill();
    }
});