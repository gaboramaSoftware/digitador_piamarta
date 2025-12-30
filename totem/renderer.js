// C:\Digitador\totem\renderer.js

const API_URL = 'http://localhost:18080';
let currentScreen = 'waiting';
let autoReturnTimeout = null;
let pollingActive = false;

// Screen elements
const screens = {
    waiting: document.getElementById('screen-waiting'),
    processing: document.getElementById('screen-processing'),
    approved: document.getElementById('screen-approved'),
    rejected: document.getElementById('screen-rejected')
};

// Verificar que las pantallas existan
console.log('[INIT] Pantallas encontradas:', {
    waiting: !!screens.waiting,
    processing: !!screens.processing,
    approved: !!screens.approved,
    rejected: !!screens.rejected
});

// ============================================
// GESTIÓN DE PANTALLAS
// ============================================

function showScreen(screenName) {
    console.log('[UI] Cambiando a pantalla:', screenName);

    Object.entries(screens).forEach(([name, screen]) => {
        if (screen) {
            screen.classList.remove('active');
        }
    });

    if (screens[screenName]) {
        screens[screenName].classList.add('active');
        currentScreen = screenName;
        console.log('[UI] ✓ Pantalla activa:', screenName);
    } else {
        console.error('[UI] ✗ Pantalla no encontrada:', screenName);
    }

    if (autoReturnTimeout) {
        clearTimeout(autoReturnTimeout);
        autoReturnTimeout = null;
    }
}

function autoReturnToWaiting(delayMs = 5000) {
    console.log(`[UI] Auto-retorno a waiting en ${delayMs}ms`);
    autoReturnTimeout = setTimeout(() => {
        showScreen('waiting');
    }, delayMs);
}

// ============================================
// MOSTRAR RESULTADOS
// ============================================

function mostrarAprobado(data) {
    console.log('[UI] Mostrando APROBADO con datos:', data);
    
    const nombre = document.getElementById('approved-nombre');
    const run = document.getElementById('approved-run');
    const curso = document.getElementById('approved-curso');
    const racion = document.getElementById('approved-racion');

    if (nombre) nombre.textContent = data?.nombre || 'N/A';
    if (run) run.textContent = data?.run || 'N/A';
    if (curso) curso.textContent = data?.curso || 'N/A';
    if (racion) racion.textContent = data?.racion || 'N/A';

    showScreen('approved');
    autoReturnToWaiting(5000);
}

function mostrarRechazado(razon, nombre = '') {
    console.log('[UI] Mostrando RECHAZADO:', razon, nombre);
    
    const reasonEl = document.getElementById('rejected-reason');
    const nombreEl = document.getElementById('rejected-nombre');

    if (reasonEl) reasonEl.textContent = razon;
    if (nombreEl) nombreEl.textContent = nombre;

    showScreen('rejected');
    autoReturnToWaiting(4000);
}

// ============================================
// COMUNICACIÓN CON EL SERVIDOR C++
// ============================================

async function verificarHuella() {
    try {
        const response = await fetch(`${API_URL}/api/verify_finger`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' }
        });

        if (!response.ok) {
            if (response.status === 503) {
                return { status: 'sensor_unavailable' };
            }
            console.error('[API] Error HTTP:', response.status);
            return null;
        }

        const data = await response.json();
        console.log('[API] Respuesta raw:', JSON.stringify(data));
        return data;

    } catch (error) {
        console.error('[API] Error de conexión:', error.message);
        return null;
    }
}

function procesarRespuesta(data) {
    if (!data) {
        return;
    }

    console.log('[PROC] Procesando:', JSON.stringify(data));
    console.log('[PROC] type:', data.type, '| status:', data.status);

    // Ticket aprobado
    if (data.type === 'ticket' && data.status === 'approved') {
        console.log('[PROC] >>> TICKET APROBADO <<<');
        mostrarAprobado(data.data);
        return;
    }

    // Ticket rechazado - ya comió
    if (data.type === 'ticket' && data.status === 'rejected_double') {
        console.log('[PROC] >>> TICKET RECHAZADO (doble) <<<');
        mostrarRechazado(
            `Ya recibió ${data.data?.racion || 'ración'} hoy`,
            data.data?.nombre || ''
        );
        return;
    }

    // Huella no reconocida
    if (data.type === 'no_match' && data.status === 'rejected') {
        console.log('[PROC] >>> HUELLA NO RECONOCIDA <<<');
        mostrarRechazado(
            'Huella no reconocida',
            'Estudiante no registrado'
        );
        return;
    }

    // Esperando huella (no hay dedo en el sensor)
    if (data.type === 'no_match' && data.status === 'waiting') {
        return;
    }

    // Sensor no disponible
    if (data.status === 'sensor_unavailable') {
        return;
    }

    // Error del servidor
    if (data.status === 'error') {
        console.error('[PROC] Error del servidor:', data.message);
        mostrarRechazado('Error del sistema', data.message || 'Contacte al administrador');
        return;
    }

    console.log('[PROC] Respuesta no manejada:', data);
}

// ============================================
// BUCLE PRINCIPAL DE POLLING
// ============================================

async function iniciarBucleHuella() {
    if (pollingActive) {
        console.log('[POLLING] Ya está activo');
        return;
    }

    pollingActive = true;
    console.log('[POLLING] ✓ Iniciando bucle de verificación...');

    while (pollingActive) {
        if (currentScreen === 'waiting') {
            const resultado = await verificarHuella();
            procesarRespuesta(resultado);
        }

        await new Promise(resolve => setTimeout(resolve, 500));
    }

    console.log('[POLLING] Bucle terminado');
}

function detenerBucleHuella() {
    pollingActive = false;
    console.log('[POLLING] Bucle detenido');
}

// ============================================
// VERIFICAR CONEXIÓN CON SERVIDOR
// ============================================

async function verificarConexion() {
    try {
        const response = await fetch(`${API_URL}/api/sensor/status`);
        const data = await response.json();
        
        console.log('[CONEXIÓN] Estado del sensor:', data);
        return data.available === true;
    } catch (error) {
        console.error('[CONEXIÓN] Error:', error.message);
        return false;
    }
}

// ============================================
// INICIALIZACIÓN
// ============================================

async function inicializar() {
    console.log('==========================================');
    console.log('[INIT] Inicializando Totem UI...');
    console.log('==========================================');
    
    showScreen('waiting');

    let intentos = 0;
    const maxIntentos = 30;

    while (intentos < maxIntentos) {
        const conectado = await verificarConexion();
        if (conectado) {
            console.log('[INIT] ✓ Conexión establecida con sensor');
            break;
        }
        intentos++;
        console.log(`[INIT] Esperando servidor... (${intentos}/${maxIntentos})`);
        await new Promise(r => setTimeout(r, 1000));
    }

    if (intentos >= maxIntentos) {
        console.error('[INIT] ✗ No se pudo conectar al servidor');
        mostrarRechazado('Error de conexión', 'No se pudo conectar al servidor');
        return;
    }

    console.log('[INIT] Iniciando polling de huella...');
    iniciarBucleHuella();
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', inicializar);
} else {
    inicializar();
}

window.addEventListener('beforeunload', () => {
    detenerBucleHuella();
});