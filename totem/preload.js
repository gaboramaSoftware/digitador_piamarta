const { contextBridge, ipcRenderer } = require('electron');

// Expose safe API to renderer
contextBridge.exposeInMainWorld('totem', {
    onBackendMessage: (callback) => {
        ipcRenderer.on('backend-message', (event, data) => {
            callback(data);
        });
    }
});
