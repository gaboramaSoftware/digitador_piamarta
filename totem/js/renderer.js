// ==========================================
// 1. FUNCIONES DE AYUDA Y CONEXIÓN (GLOBALES)
// ==========================================

// Definir Cmenu AQUÍ arriba para que exista siempre
async function Cmenu(data) {
    console.log("[API] Enviando a C++:", data);
    const response = await fetch('http://localhost:18080/api/Cmenu', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });
    return response.json();
}

// Auxiliar para espera
const wait = (ms) => new Promise(resolve => setTimeout(resolve, ms));

// Función "Ping" para esperar al servidor
async function esperarConexionCerebro(maxIntentos = 15) {
    for (let i = 0; i < maxIntentos; i++) {
        try {
            await fetch('http://localhost:18080/api/Cmenu', { 
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ping: true }) 
            });
            return true; // ¡Conectado!
        } catch (e) {
            console.log(`Intento ${i + 1}/${maxIntentos}: Esperando servidor...`);
            await wait(1000); 
        }
    }
    return false; // Tiempo agotado
}

// ==========================================
// 2. GESTIÓN DE ESTADO Y UI
// ==========================================

// State management
let currentScreen = 'waiting';
let autoReturnTimeout = null;
// Keyboard variables
let isKeyboardVisible = false;
let activeInput = null;
let keyboard = null;

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
        if (screen) screen.classList.remove('active');
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

// ==========================================
// 3. TECLADO VIRTUAL
// ==========================================

function initTouchKeyboard() {
    console.log('[UI] Initializing touch keyboard...');
    
    document.querySelectorAll('input[type="text"], input[type="number"]').forEach(input => {
        input.addEventListener('focus', () => {
            activeInput = input;
            showKeyboard();
        });
        
        input.addEventListener('blur', () => {
            // Pequeño delay para permitir clicks en el teclado
            setTimeout(() => {
                if (activeInput === input && !isKeyboardVisible) {
                    activeInput = null;
                }
            }, 100);
        });
    });
    
    // Create keyboard container if it doesn't exist
    let keyboardContainer = document.getElementById('keyboard-container');
    if (!keyboardContainer) {
        keyboardContainer = document.createElement('div');
        keyboardContainer.id = 'keyboard-container';
        keyboardContainer.className = 'keyboard-container';
        document.body.appendChild(keyboardContainer);
    }
    
    createKeyboard();
}

function createKeyboard() {
    if (keyboard) return;
    
    const keyboardLayout = {
        'default': [
            '1 2 3 4 5 6 7 8 9 0 {bksp}',
            'q w e r t y u i o p',
            'a s d f g h j k l ñ',
            '{shift} z x c v b n m , . {enter}',
            '{space}'
        ]
    };
    
    const displayMappings = {
        '{bksp}': '⌫', '{shift}': '⇧', '{enter}': '↵', '{space}': 'Espacio'
    };
    
    const keyboardContainer = document.getElementById('keyboard-container');
    let keyboardHTML = '<div class="virtual-keyboard" id="virtual-keyboard">';
    
    keyboardLayout.default.forEach(row => {
        keyboardHTML += '<div class="keyboard-row">';
        const keys = row.split(' ');
        keys.forEach(key => {
            const displayText = displayMappings[key] || key;
            const keyClass = key.startsWith('{') ? 'special-key ' + key.replace(/[{}]/g, '') : 'key';
            keyboardHTML += `<button class="${keyClass}" data-action="${key}" type="button">${displayText}</button>`;
        });
        keyboardHTML += '</div>';
    });
    keyboardHTML += '</div>';
    keyboardContainer.innerHTML = keyboardHTML;
    
    // Listeners
    document.querySelectorAll('#virtual-keyboard button').forEach(button => {
        const handleAction = (e) => {
            e.preventDefault();
            handleKeyPress(e);
        };
        button.addEventListener('click', handleAction);
        button.addEventListener('touchstart', handleAction);
    });
    
    keyboard = {
        show: () => { keyboardContainer.style.display = 'block'; isKeyboardVisible = true; },
        hide: () => { keyboardContainer.style.display = 'none'; isKeyboardVisible = false; },
        toggle: () => { isKeyboardVisible ? keyboard.hide() : keyboard.show(); }
    };
    
    keyboard.hide();
}

function handleKeyPress(event) {
    event.preventDefault();
    const button = event.currentTarget;
    const action = button.getAttribute('data-action');
    const keyText = button.textContent;
    
    if (!activeInput) return;
    const input = activeInput;
    
    switch(action) {
        case '{bksp}': input.value = input.value.slice(0, -1); break;
        case '{enter}': 
            // SIMULAR ENTER REAL
            input.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', keyCode: 13, bubbles: true }));
            keyboard.hide(); 
            break;
        case '{space}': input.value += ' '; break;
        case '{shift}': toggleShift(); break;
        default: input.value += keyText.trim(); break;
    }
    
    input.dispatchEvent(new Event('input', { bubbles: true }));
    // Opcional: sendKeyToBackend(action, keyText);
}

function toggleShift() {
    const letterKeys = document.querySelectorAll('#virtual-keyboard .key:not(.special-key)');
    letterKeys.forEach(key => {
        const current = key.textContent;
        if (current.length === 1 && current.match(/[a-zñ]/i)) {
            key.textContent = current === current.toLowerCase() ? current.toUpperCase() : current.toLowerCase();
            key.setAttribute('data-action', key.textContent);
        }
    });
}

function showKeyboard() { if (keyboard && !isKeyboardVisible) keyboard.show(); }
function hideKeyboard() { if (keyboard && isKeyboardVisible) keyboard.hide(); }

// ==========================================
// 4. COMUNICACIÓN BACKEND -> FRONTEND
// ==========================================

// Handle backend messages
if (window.totem && window.totem.onBackendMessage) {
    window.totem.onBackendMessage((data) => {
        console.log('[RENDERER] Received:', data);

        // Status updates
        if (data.status === 'ready') showScreen('waiting');
        else if (data.status === 'processing_finger') {
            showScreen('processing');
            hideKeyboard();
        }

        // Ticket events
        if (data.type === 'ticket') {
            if (data.status === 'approved') {
                document.getElementById('approved-nombre').textContent = data.data.nombre || '';
                document.getElementById('approved-run').textContent = data.data.run || '';
                document.getElementById('approved-curso').textContent = data.data.curso || '';
                document.getElementById('approved-racion').textContent = data.data.racion || '';
                showScreen('approved');
            } else if (data.status === 'rejected_double') {
                document.getElementById('rejected-reason').textContent = `Ya recibió ${data.data.racion || 'la ración'} hoy`;
                document.getElementById('rejected-nombre').textContent = data.data.nombre || '';
                showScreen('rejected');
            } else if (data.status === 'rejected_time') {
                document.getElementById('rejected-reason').textContent = 'Fuera del horario de servicio';
                document.getElementById('rejected-nombre').textContent = data.data.nombre || '';
                showScreen('rejected');
            }
            autoReturnToWaiting(5000);
        }

        // No match / Error
        if (data.type === 'no_match' || data.error) {
            if (document.getElementById('rejected-reason')) {
                document.getElementById('rejected-reason').textContent = data.error ? 'Error del sistema' : 'Huella no reconocida';
            }
            showScreen('rejected');
            autoReturnToWaiting(4000);
        }
    });
}

// ==========================================
// 5. INICIALIZACIÓN (DOMContentLoaded)
// ==========================================

document.addEventListener('DOMContentLoaded', () => {
    console.log('[RENDERER] Totem UI initialized');
    
    // Initialize screens
    Object.values(screens).forEach(screen => { if (screen) screen.classList.remove('active'); });
    showScreen('waiting');
    
    // Initialize keyboard
    initTouchKeyboard();
    
    // Toggle manual del teclado
    const toggleKeyboardBtn = document.getElementById('toggle-keyboard-btn');
    if (toggleKeyboardBtn) {
        toggleKeyboardBtn.addEventListener('click', () => { if (keyboard) keyboard.toggle(); });
    }

    // ---------------------------------------------------------
    // LOGICA DEL BOTÓN DE MENÚ (CORREGIDA Y UNIFICADA)
    // ---------------------------------------------------------
    const btnMenu = document.getElementById('btn-menu');
    
    if (btnMenu) {
        // Truco: Reemplazar el botón por un clon para borrar eventos viejos/duplicados
        const newBtn = btnMenu.cloneNode(true);
        btnMenu.parentNode.replaceChild(newBtn, btnMenu);

        newBtn.addEventListener('click', async () => {
            const textoOriginal = newBtn.innerText;
            newBtn.innerText = "⏳ Conectando...";
            newBtn.disabled = true;
            newBtn.style.opacity = "0.7";

            try {
                console.log('1. [JS] Pidiendo a Electron que abra el CMD...');
                const serverResult = await window.totem.startCppServer();
                
                if (!serverResult.success) {
                    throw new Error("Fallo al iniciar CMD: " + serverResult.message);
                }
                
                console.log('2. [JS] Esperando que el servidor HTTP responda...');
                const conectado = await esperarConexionCerebro();

                if (conectado) {
                    console.log('3. [JS] Servidor listo. Pidiendo Menú...');
                    const result = await Cmenu({ option: 1 });
                    console.log('Respuesta del Menú:', result);
                    
                    newBtn.innerText = "✅ Sistema Activo";
                    // Aquí podrías ocultar el botón
                    newBtn.style.display = 'none'; 
                } else {
                    throw new Error("Tiempo de espera agotado. El servidor no respondió.");
                }

            } catch (error) {
                console.error('Error Conexión:', error);
                newBtn.innerText = "❌ Error";
                alert(error.message);
                
                setTimeout(() => {
                    newBtn.innerText = textoOriginal;
                    newBtn.disabled = false;
                    newBtn.style.opacity = "1";
                }, 3000);
            }
        });
    }
    
    // Ocultar teclado al hacer clic fuera
    document.addEventListener('click', (e) => {
        if (isKeyboardVisible && 
            !e.target.closest('#keyboard-container') && 
            !e.target.closest('#toggle-keyboard-btn') && 
            !e.target.closest('input')) {
            hideKeyboard();
        }
    });
});