```javascript
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

let mainWindow;
let backendProcess;

const createMainWindow = () => {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        frame: false,
        backgroundColor: '#121212',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: path.join(__dirname, 'precarga.jsx')
        },
    });

    const isDev = true;
    if (isDev) {
        mainWindow.loadURL('http://localhost:5173');
    } else {
        mainWindow.loadFile(path.join(__dirname, 'frontend/dist/index.html'));
    }
};

const startBackend = () => {
    const exePath = path.join(__dirname, 'bin', 'Digitador.exe');
    console.log(`[Main] Lanzando backend: ${ exePath } `);

    backendProcess = spawn(exePath, ['--headless']);

    backendProcess.stdout.on('data', (data) => {
        const str = data.toString();
        // Puede venir más de una línea o líneas cortadas.
        // Para V1 asumimos líneas completas o usamos split básico.
        const lines = str.split('\n');
        lines.forEach(line => {
            if (!line.trim()) return;
            try {
                // Intentar parsear JSON
                if (line.trim().startsWith('{')) {
                    const json = JSON.parse(line);
                    console.log('[C++ JSON]', json);
                    if (mainWindow) {
                        // Mapear eventos de C++ a lo que espera el Frontend
                        if (json.type === 'ticket') {
                            mainWindow.webContents.send('hw:status', { type: 'MATCH', data: json.data });
                        } else if (json.type === 'no_match') {
                            mainWindow.webContents.send('hw:status', { type: 'NO_MATCH' });
                        } else if (json.status === 'processing_finger') {
                            mainWindow.webContents.send('hw:status', { type: 'LEYENDO' });
                        } else if (json.status === 'ready') {
                            mainWindow.webContents.send('hw:status', { type: 'SENSOR_LISTO' });
                        } else if (json.status === 'enroll_success') {
                             mainWindow.webContents.send('hw:status', { type: 'ENROLL_OK' }); // Definir en frontend
                        } else {
                             // Reenviar crudo si no hay mapeo específico
                             mainWindow.webContents.send('hw:status', { type: 'RAW', payload: json });
                        }
                    }
                } else {
                    console.log(`[C++ LOG] ${ line } `);
                }
            } catch (e) {
                console.log(`[C++ RAW] ${ line } `);
            }
        });
    });

    backendProcess.stderr.on('data', (data) => {
        console.error(`[C++ ERR] ${ data } `);
    });

    backendProcess.on('close', (code) => {
        console.log(`[C++] Proceso terminó con código ${ code } `);
    });
};

app.whenReady().then(() => {
    startBackend();
    createMainWindow();

    // --- HANDLERS ---

    ipcMain.on('app:minimize', () => mainWindow.minimize());
    ipcMain.on('app:close', () => mainWindow.close());

    ipcMain.handle('auth:login', async (event, { rut, pass }) => {
        console.log(`[Main] Login: ${ rut } `);
        // TODO: Implementar login real contra C++ si es necesario
        // Por ahora mock para Admin
        if (rut === 'admin' || rut === '11111111') return { success: true, role: 'admin' };
        return { success: false };
    });

    ipcMain.handle('hw:enrolar', async (event, datos) => {
        console.log('[Main] Enrolar:', datos);
        if (backendProcess) {
            const cmd = JSON.stringify({ cmd: 'enroll', run: datos.run, nombre: datos.nombre }) + '\n';
            backendProcess.stdin.write(cmd);
        }
        return { success: true };
    });

    ipcMain.handle('hw:verificar', async (event) => {
        // En modo headless, el loop ya está verificando.
        // Podríamos forzar un reset de estado visual.
        return { status: 'looping' };
    });

    ipcMain.on('hw:cancelar', () => {
        if (backendProcess) {
            backendProcess.stdin.write(JSON.stringify({ cmd: 'stop' }) + '\n');
            // Ojo: stop detiene el loop. Deberíamos tener 'cancel_enroll' vs 'stop_app'.
            // Por ahora asumimos que cancela enrolamiento y vuelve a ticket loop?
            // En V1 HeadlessManager, 'stop' mata el loop. Necesitamos 'cancel'.
            // TODO: Implementar 'cancel' en C++ si se desea volver al loop.
        }
    });
});

app.on('will-quit', () => {
    if (backendProcess) backendProcess.kill();
});
```
