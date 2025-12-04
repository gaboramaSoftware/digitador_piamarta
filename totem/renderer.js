// State management
let currentScreen = 'waiting';
let autoReturnTimeout = null;

// Screen elements
const screens = {
    waiting: document.getElementById('screen-waiting'),
    processing: document.getElementById('screen-processing'),
    approved: document.getElementById('screen-approved'),
    rejected: document.getElementById('screen-rejected')
};

// Show a specific screen
function showScreen(screenName) {
    console.log('[UI] Switching to screen:', screenName);

    // Hide all screens
    Object.values(screens).forEach(screen => {
        screen.classList.remove('active');
    });

    // Show target screen
    if (screens[screenName]) {
        screens[screenName].classList.add('active');
        currentScreen = screenName;
    }

    // Clear any pending auto-return
    if (autoReturnTimeout) {
        clearTimeout(autoReturnTimeout);
        autoReturnTimeout = null;
    }
}

// Auto-return to waiting screen after delay
function autoReturnToWaiting(delayMs = 5000) {
    autoReturnTimeout = setTimeout(() => {
        showScreen('waiting');
    }, delayMs);
}

// Handle backend messages
window.totem.onBackendMessage((data) => {
    console.log('[RENDERER] Received:', data);

    // Status updates
    if (data.status === 'ready') {
        showScreen('waiting');
    } else if (data.status === 'processing_finger') {
        showScreen('processing');
    }

    // Ticket events
    if (data.type === 'ticket') {
        if (data.status === 'approved') {
            // Show approved screen with student info
            document.getElementById('approved-nombre').textContent = data.data.nombre;
            document.getElementById('approved-run').textContent = data.data.run;
            document.getElementById('approved-curso').textContent = data.data.curso;
            document.getElementById('approved-racion').textContent = data.data.racion;

            showScreen('approved');
            autoReturnToWaiting(5000);

        } else if (data.status === 'rejected_double') {
            // Already ate this ration today
            document.getElementById('rejected-reason').textContent =
                `Ya recibió ${data.data.racion} hoy`;
            document.getElementById('rejected-nombre').textContent = data.data.nombre;

            showScreen('rejected');
            autoReturnToWaiting(5000);

        } else if (data.status === 'rejected_time') {
            // Outside service hours
            document.getElementById('rejected-reason').textContent =
                'Fuera del horario de servicio';
            document.getElementById('rejected-nombre').textContent = data.data.nombre;

            showScreen('rejected');
            autoReturnToWaiting(5000);
        }
    }

    // No match
    if (data.type === 'no_match') {
        document.getElementById('rejected-reason').textContent =
            'Huella no reconocida';
        document.getElementById('rejected-nombre').textContent =
            'Estudiante no registrado';

        showScreen('rejected');
        autoReturnToWaiting(4000);
    }

    // Errors
    if (data.error) {
        console.error('[ERROR]', data.error);
        document.getElementById('rejected-reason').textContent =
            'Error del sistema';
        document.getElementById('rejected-nombre').textContent =
            'Por favor contacte al administrador';

        showScreen('rejected');
        autoReturnToWaiting(5000);
    }
});

// Initialize
console.log('[RENDERER] Totem UI initialized');
showScreen('waiting');
