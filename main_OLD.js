//C:\Digitador\main.js
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');



let mainWindow;
let loginWindow = null; 

const createMainWindow = () => {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        frame: false,           
        backgroundColor: '#121212',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            // IMPORTANTE: Asegúrate de que este archivo exista en esta ruta exacta
            preload: path.join(__dirname, 'preload.js')
        },
    });

    const rutaProduccion = path.join(__dirname, '../index/index.html');    
    const rutaRespaldo = path.join(__dirname, '../index/index.html');

    mainWindow.loadFile(rutaProduccion).catch((err) => {
        console.log("[Error]: No se encontró index.html principal.");
        mainWindow.loadFile(rutaRespaldo);
    });

    console.log("!!! INICIANDO MAIN.JS DESDE: " + __filename);
    console.log("!!! CARPETA BASE: " + __dirname);
};

const createAuthWindow = () => {
    if (loginWindow) {
        loginWindow.focus();
        return;
    }

    loginWindow = new BrowserWindow({
        width: 400,
        height: 500,
        parent: mainWindow, 
        modal: true,        
        frame: false,
        backgroundColor: '#e1e1e1',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            // Usamos el mismo preload
            preload: path.join(__dirname, 'preload.js') 
        }
    });

    const rutaAuth = path.join(__dirname, '../index/auth.html');
    
    console.log("[MAIN] Abriendo Auth en:", rutaAuth); 
    loginWindow.loadFile(rutaAuth);
    
    loginWindow.on('closed', () => {
        loginWindow = null;
    });
};

app.whenReady().then(() => {
    createMainWindow();

    // --- ESCUCHADORES DE EVENTOS (SOLO UNO DE CADA TIPO) ---

    // 1. Abrir menú Auth
    ipcMain.on('app:open-auth', () => {
        console.log("[MAIN] ¡SEÑAL RECIBIDA! Abriendo ventana de autenticación...");
        createAuthWindow();
    });

    // 2. Cerrar app
    ipcMain.on('app:close', () => {
        if (mainWindow) mainWindow.close();
    });

    // 3. Minimizar app
    ipcMain.on('app:minimize', () => {
        if (mainWindow) mainWindow.minimize();
    });
    
    // 4. Login (Manejador)
    ipcMain.handle('auth:login', async (event, args) => {
        const { rut } = args || {};
        
        console.log("[MAIN] Intento de login con RUT:", rut);

        if (rut === 'admin' || rut === '11111111') {
            if (loginWindow) loginWindow.close(); 
            return { success: true, role: 'admin' };
        }
        
        return { success: false };
    });

}); // <--- ESTO FALTABA: Cerrar el bloque whenReady

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});