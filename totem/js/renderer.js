// renderer.js

// State management
let currentScreen = 'waiting';
let autoReturnTimeout = null;
// Keyboard variables
let isKeyboardVisible = false;
let activeInput = null;
let keyboard = null



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
        if (screen) {
            screen.classList.remove('active');
        }
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

// Initialize touch keyboard
function initTouchKeyboard() {
    console.log('[UI] Initializing touch keyboard...');
    
    // Set up input focus tracking
    document.querySelectorAll('input[type="text"], input[type="number"]').forEach(input => {
        input.addEventListener('focus', () => {
            activeInput = input;
            showKeyboard();
        });
        
        input.addEventListener('blur', () => {
            if (activeInput === input) {
                activeInput = null;
            }
        });

        console.log('[UI] Touch keyboard initialized');
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

// Create keyboard
function createKeyboard() {
    if (keyboard) return;
    
    console.log('[UI] Creating virtual keyboard...');
    
    // Create keyboard HTML structure
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
        '{bksp}': '⌫',
        '{shift}': '⇧',
        '{enter}': '↵',
        '{space}': 'Espacio'
    };
    
    const keyboardContainer = document.getElementById('keyboard-container');
    if (!keyboardContainer) return;
    
    // Create keyboard HTML
    let keyboardHTML = '<div class="virtual-keyboard" id="virtual-keyboard">';
    
    keyboardLayout.default.forEach(row => {
        keyboardHTML += '<div class="keyboard-row">';
        const keys = row.split(' ');
        
        keys.forEach(key => {
            const displayText = displayMappings[key] || key;
            const keyClass = key.startsWith('{') ? 'special-key ' + key.replace(/[{}]/g, '') : 'key';
            
            keyboardHTML += `
                <button class="${keyClass}" data-action="${key}" type="button">
                    ${displayText}
                </button>
            `;
        });
        
        keyboardHTML += '</div>';
    });
    
    keyboardHTML += '</div>';
    keyboardContainer.innerHTML = keyboardHTML;
    
    // Add event listeners to keys
    document.querySelectorAll('#virtual-keyboard button').forEach(button => {
        button.addEventListener('click', handleKeyPress);
        button.addEventListener('touchstart', (e) => {
            e.preventDefault();
            handleKeyPress(e);
        });
    });
    
    keyboard = {
        show: () => {
            keyboardContainer.style.display = 'block';
            isKeyboardVisible = true;
        },
        hide: () => {
            keyboardContainer.style.display = 'none';
            isKeyboardVisible = false;
        },
        toggle: () => {
            if (isKeyboardVisible) {
                keyboard.hide();
            } else {
                keyboard.show();
            }
        }
    };
    
    // Hide keyboard initially
    keyboard.hide();
    
    console.log('[UI] Touch keyboard initialized');


}

// Handle key press
function handleKeyPress(event) {
    event.preventDefault();
    const button = event.currentTarget;
    const action = button.getAttribute('data-action');
    const keyText = button.textContent;
    
    console.log('[UI] Key pressed:', action, keyText);
    
    if (!activeInput) return;
    
    const input = activeInput;
    
    switch(action) {
        case '{bksp}':
            // Backspace
            input.value = input.value.slice(0, -1);
            break;
            
        case '{enter}':
            // Enter - simulate pressing Enter key
            input.dispatchEvent(new KeyboardEvent('keydown', {
                key: 'Enter',
                keyCode: 13,
                bubbles: true
            }));
            keyboard.hide();
            break;
            
        case '{shift}':
            // Toggle shift - you can implement shift logic here
            toggleShift();
            break;
            
        case '{space}':
            // Space
            input.value += ' ';
            break;
            
        default:
            // Regular key
            input.value += keyText;
            break;
    }
    
    // Trigger input event
    input.dispatchEvent(new Event('input', { bubbles: true }));
    input.dispatchEvent(new Event('change', { bubbles: true }));
    
    // Send to backend if needed
    sendKeyToBackend(action, keyText);
}

function toggleShift() {
    // Toggle uppercase for letter keys
    const letterKeys = document.querySelectorAll('#virtual-keyboard .key:not(.special-key)');
    letterKeys.forEach(key => {
        const current = key.textContent;
        if (current.length === 1 && current.match(/[a-zñ]/i)) {
            key.textContent = current === current.toLowerCase() 
                ? current.toUpperCase() 
                : current.toLowerCase();
            key.setAttribute('data-action', key.textContent);
        }
    });
}

function showKeyboard() {
    if (keyboard && !isKeyboardVisible) {
        keyboard.show();
    }
}

function hideKeyboard() {
    if (keyboard && isKeyboardVisible) {
        keyboard.hide();
    }
}

function sendKeyToBackend(action, keyText) {
    // Send key press to backend through preload API
    if (window.totem && window.totem.sendToBackend) {
        window.totem.sendToBackend({
            type: 'KEY_PRESS',
            action: action,
            key: keyText,
            timestamp: Date.now(),
            source: 'virtual-keyboard'
        });
    }
}

//funcion para conectar con el cerebro C++
async function Cmenu(data) {
    const response = await fetch('http://localhost:18080/api/Cmenu', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });
    return response.json();
}

// Handle backend messages
window.totem.onBackendMessage((data) => {
    console.log('[RENDERER] Received:', data);

    // Status updates
    if (data.status === 'ready') {
        showScreen('waiting');
    } else if (data.status === 'processing_finger') {
        showScreen('processing');
        hideKeyboard(); // Hide keyboard when processing fingerprint
    }

    // Ticket events
    if (data.type === 'ticket') {
        if (data.status === 'approved') {
            // Show approved screen with student info
            document.getElementById('approved-nombre').textContent = data.data.nombre || '';
            document.getElementById('approved-run').textContent = data.data.run || '';
            document.getElementById('approved-curso').textContent = data.data.curso || '';
            document.getElementById('approved-racion').textContent = data.data.racion || '';

            showScreen('approved');
            autoReturnToWaiting(5000);

        } else if (data.status === 'rejected_double') {
            // Already ate this ration today
            document.getElementById('rejected-reason').textContent =
                `Ya recibió ${data.data.racion || 'la ración'} hoy`;
            document.getElementById('rejected-nombre').textContent = data.data.nombre || '';

            showScreen('rejected');
            autoReturnToWaiting(5000);

        } else if (data.status === 'rejected_time') {
            // Outside service hours
            document.getElementById('rejected-reason').textContent =
                'Fuera del horario de servicio';
            document.getElementById('rejected-nombre').textContent = data.data.nombre || '';

            showScreen('rejected');
            autoReturnToWaiting(5000);
        }
    }

    // No match
    if (data.type === 'no_match') {
        if (document.getElementById('rejected-reason')) {
            document.getElementById('rejected-reason').textContent =
                'Huella no reconocida';
        }
        if (document.getElementById('rejected-nombre')) {
            document.getElementById('rejected-nombre').textContent =
                'Estudiante no registrado';
        }

        showScreen('rejected');
        autoReturnToWaiting(4000);
    }

    // Errors
    if (data.error) {
        console.error('[ERROR]', data.error);
        if (document.getElementById('rejected-reason')) {
            document.getElementById('rejected-reason').textContent =
                'Error del sistema';
        }
        if (document.getElementById('rejected-nombre')) {
            document.getElementById('rejected-nombre').textContent =
                'Por favor contacte al administrador';
        }

        showScreen('rejected');
        autoReturnToWaiting(5000);
    }
});

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    console.log('[RENDERER] Totem UI initialized');
    
    // Initialize screens
    Object.values(screens).forEach(screen => {
        if (screen) {
            screen.classList.remove('active');
        }
    });
    
    // Start with waiting screen
    showScreen('waiting');
    
    // Initialize keyboard
    initTouchKeyboard();
    
    // Add click handler for manual keyboard toggle (optional)
    const toggleKeyboardBtn = document.getElementById('toggle-keyboard-btn');
    if (toggleKeyboardBtn) {
        toggleKeyboardBtn.addEventListener('click', () => {
            console.log('Click en botón, keyboard:', keyboard);
            if (keyboard) {
                keyboard.toggle();
            }
        });
    }

    //conectar a cerebro C++
    document.getElementById('btn-menu').addEventListener('click', async () => {
        
        //primero iniciar servidor C
        console.log('Iniciar servidor C++');
        const serverResult = await window.totem.startCppServer();
        console.log('Servidor: ', serverResult);
        
        try{
            const result = await Cmenu({option: 1});
            console.log('Respuesta:', result);
        }catch (error){
            console.error('Error conectando al servidor C++:', error);
        }
    });
    
    // Hide keyboard when clicking outside
    document.addEventListener('click', (e) => {
        if (isKeyboardVisible && 
        !e.target.closest('#keyboard-container') && 
        !e.target.closest('#toggle-keyboard-btn') &&  // ← AGREGAR ESTO
        !e.target.closest('input[type="text"], input[type="number"]')) {
        hideKeyboard();
    }
    });
});