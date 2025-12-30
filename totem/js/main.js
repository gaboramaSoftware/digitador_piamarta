// C:\Digitador\totem\js\main.js
const { app, BrowserWindow, screen, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');
const http = require('http');

// --- VARIABLES ---
let mainWindow;
let loginWindow = null;
let cerebroProcess = null;

const preloadPath = path.join(__dirname, 'preload.js');

// ==========================================
// 1. CEREBRO C++ (WEBSERVER MODE)
// ==========================================
function startCerebro() {
    if (cerebroProcess) return;
    
    console.log("[MAIN] Iniciando Cerebro C++...");
    const cerebroPath = 'C:\\Digitador\\src\\Digitador.exe';

    try {
        // Iniciar con --webserver directamente (sin menú interactivo)
        cerebroProcess = spawn(cerebroPath, ['--webserver'], {
            cwd: 'C:\\Digitador\\src\\',
            shell: false,
            stdio: ['ignore', 'pipe', 'pipe'] // No necesitamos stdin
        });

        cerebroProcess.stdout.on('data', (data) => {
            console.log(`[C++]: ${data.toString().trim()}`);
        });

        cerebroProcess.stderr.on('data', (data) => {
            console.error(`[C++ ERR]: ${data.toString().trim()}`);
        });

        cerebroProcess.on('close', (code) => {
            console.log(`[MAIN] Cerebro terminó con código ${code}`);
            cerebroProcess = null;
        });

        cerebroProcess.on('error', (err) => {
            console.error("[MAIN] Error al iniciar Cerebro:", err);
            cerebroProcess = null;
        });

    } catch (e) {
        console.error("[MAIN] Excepción al lanzar Cerebro:", e);
    }
}

// Esperar a que el servidor esté listo
async function waitForServer(maxAttempts = 30) {
    console.log("[MAIN] Esperando a que el servidor C++ esté listo...");
    
    for (let i = 0; i < maxAttempts; i++) {
        try {
            const ready = await new Promise((resolve, reject) => {
                const req = http.get('http://localhost:18080/api/sensor/status', (res) => {
                    let data = '';
                    res.on('data', chunk => data += chunk);
                    res.on('end', () => {
                        try {
                            const json = JSON.parse(data);
                            resolve(json.available === true);
                        } catch (e) {
                            resolve(false);
                        }
                    });
                });
                req.on('error', () => resolve(false));
                req.setTimeout(2000, () => {
                    req.destroy();
                    resolve(false);
                });
            });

            if (ready) {
                console.log("[MAIN] ✓ Servidor C++ listo!");
                return true;
            }
        } catch (e) {
            // Ignorar errores, seguir intentando
        }
        
        console.log(`[MAIN] Esperando servidor... (${i + 1}/${maxAttempts})`);
        await new Promise(r => setTimeout(r, 1000));
    }
    
    console.error("[MAIN] ✗ Servidor no respondió");
    return false;
}

// ==========================================
// 2. VENTANA PRINCIPAL
// ==========================================
function createMainWindow() {
    const displays = screen.getAllDisplays();
    const display = displays[0]; // Usar primera pantalla
    const { width, height } = display.bounds;

    mainWindow = new BrowserWindow({
        width: width,
        height: height,
        x: display.bounds.x,
        y: display.bounds.y,
        fullscreen: !process.argv.includes('--dev'),
        frame: false,
        backgroundColor: '#1a1a2e',
        webPreferences: {
            preload: preloadPath,
            contextIsolation: true,
            nodeIntegration: false,
        }
    });

    // Cargar index.html desde la carpeta totem
    const rutaIndex = path.join(__dirname, '..', 'index.html');
    mainWindow.loadFile(rutaIndex);

    // DevTools solo en modo desarrollo
    if (process.argv.includes('--dev')) {
        mainWindow.webContents.openDevTools({ mode: 'detach' });
    }

    mainWindow.on('closed', () => {
        mainWindow = null;
    });
}

// ==========================================
// 3. VENTANA DE LOGIN (MANTENIMIENTO)
// ==========================================
function createAuthWindow() {
    if (loginWindow) {
        loginWindow.focus();
        return;
    }

    const display = screen.getPrimaryDisplay();
    const { width, height } = display.bounds;

    loginWindow = new BrowserWindow({
        width: width,
        height: height,
        parent: mainWindow,
        fullscreen: true,
        modal: true,
        frame: false,
        backgroundColor: '#e1e1e1',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: preloadPath
        }
    });

    const authPath = path.join(__dirname, '..', 'index', 'auth.html');
    loginWindow.loadFile(authPath);

    loginWindow.on('closed', () => {
        loginWindow = null;
    });
}

// ==========================================
// 4. INICIALIZACIÓN
// ==========================================
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
        
        // Credenciales de administrador
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