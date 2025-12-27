//main.js 
const { app, BrowserWindow, screen, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');

let cppServer = null;

const preloadPath = path.join(__dirname, 'preload.js'); 

console.log('Preload path:', preloadPath);

//iniciar servidor C++
ipcMain.handle('start-cpp-server', async () =>{
    if(cppServer){
        if (cppServer) return { success: true, message: 'Servidor ya está corriendo' };
        console.log("El cerebro ya esta activo.");
    }

    try {
        console.log("Iniciando Cerebro C++ vía CMD...");
        cppServer = spawn(
            'cmd.exe', 
            ['/c', '"C:\\Digitador\\src\\build&Run.bat"', '--webserver'], 
            { 
                windowsVerbatimArguments: true
            }
        );
        
        cppServer.stdout.on('data', (data) => {
            console.log(`[C++]: ${data}`);
        });
        
        cppServer.stderr.on('data', (data) => {
            console.error(`[C++ Error]: ${data}`);
        });
        
        cppServer.on('close', (code) => {
            console.log(`[C++] Servidor cerrado con código: ${code}`);
            cppServer = null;
        });
        
        // Esperamos un poco para asegurar que el CMD arrancó el proceso hijo
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        return { success: true, message: 'CMD iniciado correctamente' };

    } catch (error) {
        console.error("Error en spawn:", error);
        return { success: false, message: error.message };
    }
});

function getBestDisplay() {
    const displays = screen.getAllDisplays();
    
    // Si solo hay una pantalla, usar esa
    if (displays.length === 1) {
        console.log('📺 Usando única pantalla disponible');
        return displays[0];
    }
    
    // Buscar pantalla táctil (si tiene información de tipo)
    const touchScreen = displays.find(d => 
        d.touchSupport === 'available' || 
        d.size.width <= 1920 // Pantallas pequeñas suelen ser táctiles
    );
    
    if (touchScreen) {
        console.log('👆 Pantalla táctil detectada');
        return touchScreen;
    }
    
    // Usar pantalla primaria por defecto
    console.log('📺 Usando pantalla primaria');
    return screen.getPrimaryDisplay();
}

function createWindow() {
    const display = getBestDisplay();
    const { width, height } = display.bounds;
    
    console.log(`🖥️ Pantalla seleccionada: ${width}x${height}px`);
    console.log(`📍 Posición: x=${display.bounds.x}, y=${display.bounds.y}`);
    
    mainWindow = new BrowserWindow({
        width: width,
        height: height,
        x: display.bounds.x,  // Posicionar en la pantalla correcta
        y: display.bounds.y,
        
        // Configuración kiosko completa
        fullscreen: true,
        kiosk: process.argv.includes('--kiosk'), // Modo kiosko verdadero
        frame: false,
        resizable: false,
        alwaysOnTop: process.argv.includes('--kiosk'),
        skipTaskbar: process.argv.includes('--kiosk'),
        
        // Rendimiento
        show: false, // No mostrar hasta que esté listo
        backgroundColor: '#1a1a2e',
        
        webPreferences: {
            preload: preloadPath,  
            contextIsolation: true,
            nodeIntegration: false,
            devTools: !process.argv.includes('--kiosk')
        }
    });

    // Ocultar menú siempre
    mainWindow.setMenu(null);
    
    // Mostrar cuando esté listo
    mainWindow.once('ready-to-show', () => {
        mainWindow.show();
        
        // Forzar foco y pantalla completa
        mainWindow.focus();
        mainWindow.setFullScreen(true);
        
        // En modo kiosko, bloquear salida
        if (process.argv.includes('--kiosk')) {
            mainWindow.setKiosk(true);
            // Deshabilitar atajos de teclado
            mainWindow.webContents.on('before-input-event', (event, input) => {
                if (input.key === 'F11' || input.key === 'Escape' || 
                    (input.control && input.key === 'r')) {
                    event.preventDefault();
                }
            });
        }
    });

    mainWindow.loadFile('index/index.html');
    
    // DevTools solo en desarrollo
    if (!process.argv.includes('--kiosk') && !app.isPackaged) {
        mainWindow.webContents.openDevTools({ mode: 'detach' });
    }
}

app.whenReady().then(createWindow);

// Manejar múltiples pantallas dinámicamente
app.on('ready', () => {
    screen.on('display-added', (event, newDisplay) => {
        console.log('➕ Nueva pantalla conectada');
        // Reconfigurar si estamos en la pantalla principal
        const currentBounds = mainWindow.getBounds();
        const displays = screen.getAllDisplays();
        
        // Si nuestra ventana no está en ninguna pantalla visible
        const isOnVisibleDisplay = displays.some(d => 
            d.bounds.x <= currentBounds.x && 
            currentBounds.x < d.bounds.x + d.bounds.width &&
            d.bounds.y <= currentBounds.y && 
            currentBounds.y < d.bounds.y + d.bounds.height
        );
        
        if (!isOnVisibleDisplay) {
            console.log('🔄 Ventana fuera de pantalla, reubicando...');
            const display = getBestDisplay();
            mainWindow.setBounds(display.bounds);
        }
    });
});