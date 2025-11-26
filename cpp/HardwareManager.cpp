#include "HardwareManager.hpp"
#include "DB_Backend.hpp" // (O DB_Access_Layer.hpp)

// --- SDK y Librerías Estándar ---
// (Asegúrate que la ruta a tu SDK sea la correcta)
#include "libs/include/libzkfp.h" 
#include <iostream>
#include <stdexcept> // Para std::runtime_error
#include <map>       // Para el caché
#include <thread>
#include <chrono>

// Tamaño estándar de un template de ZKTeco
const int ZKFP_TEMPLATE_SIZE = 2048;

// ========================================================
// --- 1. Implementación del RAII Helper (DevCloser) ---
// ========================================================

// (Esta es la versión CORREGIDA. Llama a fp_close)
void DevCloser::operator()(fp_device* device) const {
    if (device) {
        fp_close(device); // <-- ¡La función C del SDK que faltaba!
        std::cout << "[Hardware] Sensor ZKTeco cerrado." << std::endl;
    }
}

// ========================================================
// --- 2. Constructor / Destructor ---
// ========================================================

// (Esta es la versión CORREGIDA, con la sintaxis correcta)
HardwareManager::HardwareManager(DB_Backend* db_manager)
    : db_manager_(db_manager), 
      dev_handle_(nullptr), // unique_ptr se inicializa a null
      dispositivosListos_(false) 
{
    if (!db_manager_) {
        // Error fatal inmediato si no nos pasan la DB
        throw std::runtime_error("HardwareManager no puede iniciar sin db_manager_");
    }
    std::cout << "[Hardware] Gestor creado." << std::endl;
}

HardwareManager::~HardwareManager() {
    // ¡Nada que hacer!
    // dev_handle_ (unique_ptr) llamará a DevCloser automáticamente.
    std::cout << "[Hardware] Gestor destruido." << std::endl;
}

// ========================================================
// --- 3. La Función de Arranque (¡La más importante!) ---
// ========================================================

bool HardwareManager::Inicializar_Dispositivos() {
    try {
        // (Paso 1: Cargar Config de DB)
        if (!_cargarPersistencia()) {
            std::cerr << "[Hardware] ERROR: Falló _cargarPersistencia." << std::endl;
            return false;
        }

        // (Paso 2: Abrir ZKTeco y cargar Caché)
        if (!_abrirZKTeco()) {
            std::cerr << "[Hardware] ERROR: Falló _abrirZKTeco." << std::endl;
            return false;
        }

        // (Paso 3: Abrir Impresora, etc.)
        if (!_abrirPuertos()) {
            std::cerr << "[Hardware] ERROR: Falló _abrirPuertos." << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "[Hardware] EXCEPCIÓN FATAL durante la inicialización: " << e.what() << std::endl;
        return false;
    }

    std::cout << "[Hardware] ¡Dispositivos listos!" << std::endl;
    dispositivosListos_ = true;
    return true;
}

// ========================================================
// --- 4. Helpers Privados (El trabajo de Init) ---
// ========================================================

// (Esta es la versión CORREGIDA. Carga 'impresoraPuerto' y retorna 'true')
bool HardwareManager::_cargarPersistencia() {
    std::cout << "[Hardware] Cargando configuración desde la DB..." << std::endl;
    
    if (!db_manager_) {
        std::cerr << "[Hardware] ERROR CRÍTICO: db_manager_ es nulo." << std::endl;
        return false;
    }

    // (Asegúrate que tu DB_Backend usa 'impresoraPuerto' en el SQL)
    std::optional<Configuracion> configOpt = db_manager_->Obtener_Configuracion();

    if (!configOpt) {
        std::cerr << "[Hardware] ERROR: No se pudo obtener la configuración de la DB." << std::endl;
        return false;
    }

    Configuracion config = configOpt.value();

    // Guardamos AMBOS valores del refactor V2
    this->idTerminal_ = config.id_terminal;
    this->impresoraPuerto_ = config.impresoraPuerto; // ¡Tu nombre!

    if (this->idTerminal_.empty() || this->impresoraPuerto_.empty()) {
        std::cerr << "[Hardware] ERROR: El id_terminal o impresoraPuerto de la DB está vacío." << std::endl;
        return false;
    }

    std::cout << "[Hardware] Config cargada. ID Terminal: " << this->idTerminal_ << std::endl;
    std::cout << "[Hardware] Puerto de Impresora: " << this->impresoraPuerto_ << std::endl;
    
    return true; // <-- ¡El bug lógico estaba aquí!
}

// (Esta es la implementación COMPLETA de _abrirZKTeco)
bool HardwareManager::_abrirZKTeco() {
    std::cout << "[Hardware] Inicializando sensor ZKTeco..." << std::endl;

    // 1. ABRIR EL DISPOSITIVO (fp_open(0) = primer USB)
    fp_device* h = fp_open(0);
    if (!h) {
        std::cerr << "[Hardware] ERROR: No se pudo abrir ZKTeco (fp_open)." << std::endl;
        return false;
    }

    // 2. ASIGNAR AL PUNTERO RAII
    this->dev_handle_.reset(h);
    std::cout << "[Hardware] Sensor ZKTeco abierto." << std::endl;

    // 3. CARGAR CACHÉ DE DB (Paso 1 del "Modo Pro")
    std::cout << "[Hardware] Cargando caché de huellas desde DB..." << std::endl;
    
    std::map<std::string, std::vector<uint8_t>> cache_templates;
    if (!db_manager_->Obtener_Todos_Templates(cache_templates)) {
        std::cerr << "[Hardware] ERROR: No se pudo cargar el caché de huellas desde la DB." << std::endl;
        return false;
    }

    if (cache_templates.empty()) {
        std::cout << "[Hardware] ADVERTENCIA: La DB no tiene huellas de estudiantes." << std::endl;
        return true; // No es un error fatal
    }

    // 4. CARGAR CACHÉ AL SDK (Paso 2 del "Modo Pro")
    int rc_init = fp_db_init(dev_handle_.get()); // Limpia la RAM del SDK
    if (rc_init != 0) {
        std::cerr << "[Hardware] ERROR: No se pudo inicializar la RAM del SDK (fp_db_init)." << std::endl;
        return false;
    }

    int templates_cargados = 0;
    for (const auto& par : cache_templates) {
        const std::string& run_id = par.first;
        const std::vector<uint8_t>& tpl = par.second;

        // Carga el par (ID, Huella) en la RAM del sensor
        int rc_add = fp_db_add(dev_handle_.get(), run_id.c_str(), tpl.data(), tpl.size());
        if (rc_add == 0) {
            templates_cargados++;
        } else {
             std::cerr << "[Hardware] ADVERTENCIA: No se pudo cargar template para RUN: " << run_id 
                      << " (Error SDK: " << rc_add << ")" << std::endl;
        }
    }

    std::cout << "[Hardware] " << templates_cargados << "/" << cache_templates.size() 
              << " templates cargados en la RAM del SDK." << std::endl;
              
    return true;
}

// (Esta es la implementación "Stub" de _abrirPuertos)
bool HardwareManager::_abrirPuertos() {
    std::cout << "[Hardware] (STUB) Abriendo puerto de impresora..." << std::endl;

    if (this->impresoraPuerto_.empty()) {
        std::cerr << "[Hardware] ERROR: No hay 'impresoraPuerto_' cargado." << std::endl;
        return false;
    }

    std::cout << "[Hardware] (STUB) Conectando a impresora en: " 
              << this->impresoraPuerto_ << std::endl;

    // -------------------------------------------------------------------
    // TODO: AQUI VA LA LÓGICA REAL PARA ABRIR EL PUERTO SERIE/USB
    // (Ej: CreateFileA, fopen, libserialport::SerialPort::Open)
    // -------------------------------------------------------------------

    std::cout << "[Hardware] (STUB) Puerto de impresora abierto con éxito." << std::endl;
    return true;
}


// ========================================================
// --- 5. Funciones Públicas (Operación) ---
// ========================================================

// (Implementación COMPLETA de CapturarHuella)
bool HardwareManager::CapturarHuella(std::vector<uint8_t>& out_tpl) {
    if (!dispositivosListos_ || !dev_handle_) {
        std::cerr << "[Hardware] ERROR: CapturarHuella llamado antes de Init." << std::endl;
        return false;
    }

    // Aseguramos que el vector de salida tenga el tamaño correcto
    out_tpl.resize(ZKFP_TEMPLATE_SIZE);
    unsigned int tpl_size = ZKFP_TEMPLATE_SIZE;
    
    // Es bloqueante: espera hasta que el usuario ponga el dedo.
    int rc = fp_capture(dev_handle_.get(), out_tpl.data(), &tpl_size);

    if (rc != 0) {
        std::cerr << "[Hardware] Captura de huella falló. (Error SDK: " << rc << ")" << std::endl;
        return false;
    }

    // Ajustamos el tamaño del vector al tamaño real del template
    out_tpl.resize(tpl_size);
    return true;
}

// (Implementación COMPLETA de IdentificarHuellaEnCache)
std::string HardwareManager::IdentificarHuellaEnCache(const std::vector<uint8_t>& tpl_capturada) {
    if (!dispositivosListos_ || !dev_handle_) {
        std::cerr << "[Hardware] ERROR: IdentificarHuella llamado antes de Init." << std::endl;
        return "";
    }
    
    char run_id_buffer[64]; 
    unsigned int score = 0;
    
    // Compara 1 template (tpl_capturada) contra N templates (en la RAM del SDK)
    int rc = fp_db_identify(
        dev_handle_.get(),
        tpl_capturada.data(),
        tpl_capturada.size(),
        run_id_buffer, // Buffer de salida para el ID
        64,            // Tamaño del buffer de salida
        &score         // Puntero de salida para el "score"
    );

    if (rc == 0) {
        // ¡ÉXITO!
        std::cout << "[Hardware] Huella identificada. RUN: " << run_id_buffer 
                  << ", Score: " << score << std::endl;
        return std::string(run_id_buffer);
    } else {
        // "no encontrado"
        std::cout << "[Hardware] Huella NO identificada (rc=" << rc << ")." << std::endl;
        return ""; // String vacío
    }
}

// (Implementación COMPLETA de VerificarCoincidencia)
bool HardwareManager::VerificarCoincidencia(
    const std::vector<uint8_t>& tpl1, 
    const std::vector<uint8_t>& tpl2
) {
    if (!dispositivosListos_ || !dev_handle_) return false;

    int score = fp_match(
        dev_handle_.get(),
        tpl1.data(), tpl1.size(),
        tpl2.data(), tpl2.size()
    );

    if (score >= UMBRAL_COINCIDENCIA) {
         std::cout << "[Hardware] Verificación 1:1 exitosa (Score: " << score << ")" << std::endl;
         return true;
    } else {
         std::cout << "[Hardware] Verificación 1:1 falló (Score: " << score << ")" << std::endl;
         return false;
    }
}


// ========================================================
// --- 6. Funciones Públicas (Stubs) ---
// ========================================================

// (Implementación "Stub" de ImprimirTicket)
bool HardwareManager::ImprimirTicket(
    const PerfilEstudiante& /*perfil*/, 
    const RegistroRacion& /*racion*/
) {
    if (!dispositivosListos_) return false;
    
    std::cout << "[Hardware] (STUB) Imprimiendo ticket en " << this->impresoraPuerto_ << "..." << std::endl;
    // -------------------------------------------------
    // TODO: Lógica de impresión real
    // (Ej: fprintf(impresora_handle_, "...") o
    //      serial_port.Write("...") )
    // -------------------------------------------------
    return true; 
}

// (Implementación "Stub" de ActivarSemaforoGui)
void HardwareManager::ActivarSemaforoGui(
    EstadoSemaforo estado, 
    const std::string& mensaje
) {
    // Esto es solo un log.
    switch(estado) {
        case EstadoSemaforo::APROBADO_PAE:
        case EstadoSemaforo::APROBADO_NORMAL:
            std::cout << "[Hardware] (GUI-STUB) LUZ VERDE: " << mensaje << std::endl;
            break;
        case EstadoSemaforo::RECHAZADO_DOBLE:
        case EstadoSemaforo::RECHAZADO_NO_PAE:
        case EstadoSemaforo::RECHAZADO_NO_EXISTE:
            std::cerr << "[Hardware] (GUI-STUB) LUZ ROJA: " << mensaje << std::endl;
            break;
    }
    
}