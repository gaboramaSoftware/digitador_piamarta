//C:\Digitador\totem\main.js
const { app, BrowserWindow } = require('electron');
const path = require('path');
const http = require('http');

let mainWindow;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 600,
        height: 800,
        fullscreen: process.argv.includes('--kiosk'),
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false
        },
        autoHideMenuBar: true,
        backgroundColor: '#1a1a1a'
    });

    mainWindow.loadFile('index.html');

    // Open DevTools in development mode
    if (!process.argv.includes('--kiosk')) {
        mainWindow.webContents.openDevTools();
    }
}

// Verificar si el servidor C++ está corriendo
async function checkServerReady(maxAttempts = 30) {
    for (let i = 0; i < maxAttempts; i++) {
        try {
            await new Promise((resolve, reject) => {
                const req = http.get('http://localhost:18080/api/sensor/status', (res) => {
                    let data = '';
                    res.on('data', chunk => data += chunk);
                    res.on('end', () => {
                        try {
                            const json = JSON.parse(data);
                            console.log('[MAIN] Estado del sensor:', json);
                            resolve(json);
                        } catch (e) {
                            resolve({ available: false });
                        }
                    });
                });
                req.on('error', reject);
                req.setTimeout(2000, () => {
                    req.destroy();
                    reject(new Error('Timeout'));
                });
            });
            console.log('[MAIN] ✓ Servidor C++ conectado!');
            return true;
        } catch (e) {
            console.log(`[MAIN] Esperando servidor C++... (${i + 1}/${maxAttempts})`);
            await new Promise(r => setTimeout(r, 1000));
        }
    }
    return false;
}

app.whenReady().then(async () => {
    console.log('[MAIN] Iniciando Totem UI...');
    console.log('[MAIN] Verificando conexión con servidor C++...');
    
    // Verificar que el servidor esté corriendo
    const serverReady = await checkServerReady();
    
    if (!serverReady) {
        console.error('[MAIN] ✗ No se pudo conectar al servidor C++');
        console.error('[MAIN] Asegúrate de iniciar Digitador.exe --webserver primero');
        // Mostrar ventana de todos modos para que el usuario vea el error
    }
    
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});