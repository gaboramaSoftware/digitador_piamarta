//C:\Digitador\totem\preload.js
const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('totem', {
    // Por si necesitas comunicación IPC en el futuro
    onBackendMessage: (callback) => {
        ipcRenderer.on('backend-message', (event, data) => {
            callback(data);
        });
    },
    
    // Información del entorno
    platform: process.platform,
    
    // Puedes agregar más funciones expuestas aquí si las necesitas
});

console.log('[PRELOAD] Cargado correctamente');