const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

let mainWindow;

const createMainWindow = () => {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        frame: false,           // Sin marco (estilo kiosco)
        backgroundColor: '#121212',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: path.join(__dirname, 'precarga.jsx')
        },
    });

    const rutaProduccion = path.join(__dirname, 'totem/index/index.html');
    
    const rutaRespaldo = path.join(__dirname, 'totem/index/index.html');

    mainWindow.loadFile(rutaProduccion).catch((err) => {
        console.log("[Error]: No se encontró frontend/dist/index.html, buscando respaldo...");
        mainWindow.loadFile(rutaRespaldo);
    });

};



app.whenReady().then(() => {
    createMainWindow();

    ipcMain.on('app:close', () => {
        if (mainWindow) mainWindow.close();
    });

    ipcMain.on('app:minimize', () => {
        if (mainWindow) mainWindow.minimize();
    });

    ipcMain.handle('start-cpp-server', async () => {
        console.log("[Main] Solicitud de inicio C++ recibida. Ignorando (gestión externa).");
        return { success: true, message: 'Gestionado por INICIAR_TODO.bat' };
    });
    
    ipcMain.handle('auth:login', async (event, args) => {
        const { rut } = args || {};
        if (rut === 'admin' || rut === '11111111') return { success: true, role: 'admin' };
        return { success: false };
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});