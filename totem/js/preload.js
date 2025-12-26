// preload.js 
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('totem', {
    onBackendMessage: (callback) => {
        ipcRenderer.on('backend-message', (event, data) => {
            console.log('[PRELOAD] Received from backend:', data);
            callback(data);
        });
    },
    
    sendToBackend: (data) => {
        console.log('[PRELOAD] Sending to backend:', data);
        ipcRenderer.send('frontend-message', data);
    },
    
    // Para desarrollo
    isDev: () => {
        return !process.argv.includes('--kiosk');
    },
    //iniciar servidor en C
    startCppServer: () => ipcRenderer.invoke('start-cpp-server')
});