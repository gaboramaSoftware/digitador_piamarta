const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('sys', {
    // --- Ventana ---
    minimizar: () => ipcRenderer.send('app:minimize'),
    cerrar: () => ipcRenderer.send('app:close'),

    // --- Auth ---
    login: (rut, pass) => ipcRenderer.invoke('auth:login', { rut, pass }),

    // --- Hardware / Negocio ---
    enrolar: (datos) => ipcRenderer.invoke('hw:enrolar', datos),
    verificar: () => ipcRenderer.invoke('hw:verificar'),
    crearBoleta: (idAlumno) => ipcRenderer.invoke('hw:boleta', idAlumno),
    cancelar: () => ipcRenderer.send('hw:cancelar'),

    // --- Escuchar Eventos (C++ -> React) ---
    // Ej: "Dedo detectado", "Error de lectura"
    onStatus: (callback) => {
        const sub = (e, msg) => callback(msg);
        ipcRenderer.on('hw:status', sub);
        return () => ipcRenderer.removeListener('hw:status', sub);
    }
});