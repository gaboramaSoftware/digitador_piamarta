const { app, BrowserWindow, ipcMain } = require('electron');
const { spawn } = require('child_process');
const path = require('path');

let mainWindow;
let backendProcess;

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

function startBackend() {
    const backendPath = path.join(__dirname, '..', 'src', 'Digitador.exe');

    console.log('[TOTEM] Starting backend:', backendPath);

    backendProcess = spawn(backendPath, ['--headless'], {
        cwd: path.join(__dirname, '..', 'src')
    });

    // Listen to stdout for JSON responses
    backendProcess.stdout.on('data', (data) => {
        const lines = data.toString().split('\n');

        lines.forEach(line => {
            line = line.trim();
            if (!line) return;

            try {
                const json = JSON.parse(line);
                console.log('[BACKEND →]', json);

                // Forward to renderer
                if (mainWindow && !mainWindow.isDestroyed()) {
                    mainWindow.webContents.send('backend-message', json);
                }
            } catch (e) {
                // Not JSON, probably debug output
                console.log('[BACKEND]', line);
            }
        });
    });

    backendProcess.stderr.on('data', (data) => {
        console.error('[BACKEND ERROR]', data.toString());
    });

    backendProcess.on('close', (code) => {
        console.log(`[BACKEND] Process exited with code ${code}`);
    });
}

app.whenReady().then(() => {
    createWindow();
    startBackend();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    // Kill backend process
    if (backendProcess && !backendProcess.killed) {
        backendProcess.kill();
    }

    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('will-quit', () => {
    // Ensure backend is killed
    if (backendProcess && !backendProcess.killed) {
        backendProcess.kill();
    }
});
