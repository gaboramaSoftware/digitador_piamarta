#include "HardwareManager.hpp"
#include "DB_Backend.hpp"
#include "SDK.hpp"

#include <iostream>
#include <stdexcept>
#include <map>
#include <optional>   // std::optional

// Tamaño estándar aproximado de un template de ZKTeco
constexpr int ZKFP_TEMPLATE_SIZE = 2048;

// Umbral de coincidencia (score del SDK)
constexpr int COINCIDENCIA = 65;

// ========================================================
// 1. RAII Helper (DevCloser)
// ========================================================

void DevCloser::operator()(fp_device* device) const {
    if (device) {
        fp_close(device); 
        std::cout << "[Hardware] Sensor ZKTeco cerrado." << std::endl;
    }
}

// ========================================================
// 2. Constructor / Destructor
// ========================================================

HardwareManager::HardwareManager(DB_Backend* db_manager)
    : db_manager_(db_manager),
      dev_handle_(nullptr),
      dispositivosListos_(false)
{
    if (!db_manager_) {
        throw std::runtime_error(
            "HardwareManager no puede iniciar sin db_manager_"
        );
    }
    std::cout << "[Hardware] Gestor creado." << std::endl;
}

HardwareManager::~HardwareManager() {
    // dev_handle_ libera solo el fp_device vía DevCloser.
    std::cout << "[Hardware] Gestor destruido." << std::endl;
}

// ========================================================
// 3. Inicialización principal
// ========================================================

bool HardwareManager::Inicializar_Dispositivos() {
    try {
        // Paso 1: Cargar configuración desde la DB
        if (!_cargarPersistencia()) {
            std::cerr << "[Hardware] ERROR: Falló _cargarPersistencia." << std::endl;
            return false;
        }

        // Paso 2: Abrir sensor ZKTeco y cargar caché de huellas
        if (!_abrirZKTeco()) {
            std::cerr << "[Hardware] ERROR: Falló _abrirZKTeco." << std::endl;
            return false;
        }

        // Paso 3: Abrir puertos (impresora u otros)
        if (!_abrirPuertos()) {
            std::cerr << "[Hardware] ERROR: Falló _abrirPuertos." << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "[Hardware] EXCEPCIÓN FATAL durante la inicialización: "
                  << e.what() << std::endl;
        return false;
    }

    std::cout << "[Hardware] ¡Dispositivos listos!" << std::endl;
    dispositivosListos_ = true;
    return true;
}

// ========================================================
// 4. Helpers privados
// ========================================================

// Lee la fila única de ConfiguracionGlobal
bool HardwareManager::_cargarPersistencia() {
    std::cout << "[Hardware] Cargando configuración desde la DB..." << std::endl;

    if (!db_manager_) {
        std::cerr << "[Hardware] ERROR CRÍTICO: db_manager_ es nulo." << std::endl;
        return false;
    }

    std::optional<Configuracion> configOpt = db_manager_->Obtener_Configuracion();
    if (!configOpt) {
        std::cerr << "[Hardware] ERROR: No se pudo obtener la configuración de la DB."
                  << std::endl;
        return false;
    }

    Configuracion config = *configOpt;

    this->idTerminal_      = config.id_terminal;
    this->impresoraPuerto_ = config.puerto_impresora;

    if (this->idTerminal_.empty() || this->impresoraPuerto_.empty()) {
        std::cerr << "[Hardware] ERROR: id_terminal o puerto_impresora vacíos en la DB."
                  << std::endl;
        return false;
    }

    std::cout << "[Hardware] Config cargada. ID Terminal: " << this->idTerminal_ << "\n"
              << "[Hardware] Puerto de Impresora: " << this->impresoraPuerto_ << std::endl;

    return true;
}

// Abre el dispositivo ZKTeco y carga las huellas en la RAM del SDK
bool HardwareManager::_abrirZKTeco() {
    std::cout << "[Hardware] Inicializando sensor ZKTeco..." << std::endl;

    // 1) Abrir el dispositivo (primer lector: index 0)
    fp_device* h = fp_open(0);
    if (!h) {
        std::cerr << "[Hardware] ERROR: No se pudo abrir ZKTeco (fp_open)." << std::endl;
        return false;
    }

    // 2) Entregamos el fp_device al unique_ptr con RAII
    this->dev_handle_.reset(h);
    std::cout << "[Hardware] Sensor ZKTeco abierto." << std::endl;

    // 3) Inicializar la DB interna del SDK (RAM)
    int rc_init = fp_db_init(dev_handle_.get());
    if (rc_init != 0) {
        std::cerr << "[Hardware] ERROR: No se pudo inicializar la RAM del SDK (fp_db_init)."
                  << std::endl;
        return false;
    }

    // 4) Cargar huellas desde tu base de datos
    std::cout << "[Hardware] Cargando caché de huellas desde DB..." << std::endl;

    std::map<std::string, std::vector<uint8_t>> cache_templates;
    if (!db_manager_->Obtener_Todos_Templates(cache_templates)) {
        std::cerr << "[Hardware] ERROR: No se pudo cargar el caché de huellas desde la DB."
                  << std::endl;
        return false;
    }

    if (cache_templates.empty()) {
        std::cout << "[Hardware] ADVERTENCIA: La DB no tiene huellas de estudiantes."
                  << std::endl;
        // No es error fatal, el sistema puede seguir pero no identificará a nadie
        return true;
    }

    int templates_cargados = 0;
    for (const auto& par : cache_templates) {
        const std::string& run_id = par.first;
        const std::vector<uint8_t>& tpl = par.second;

        int rc_add = fp_db_add(
            dev_handle_.get(),
            run_id.c_str(),
            tpl.data(),
            tpl.size()
        );

        if (rc_add == 0) {
            ++templates_cargados;
        } else {
            std::cerr << "[Hardware] ADVERTENCIA: No se pudo cargar template para RUN "
                      << run_id << " (Error SDK: " << rc_add << ")" << std::endl;
        }
    }

    std::cout << "[Hardware] " << templates_cargados << "/"
              << cache_templates.size()
              << " templates cargados en la RAM del SDK." << std::endl;

    return true;
}

// Stub: abrir puertos para impresora, etc.
bool HardwareManager::_abrirPuertos() {
    std::cout << "[Hardware] (STUB) Abriendo puerto de impresora..." << std::endl;

    if (this->impresoraPuerto_.empty()) {
        std::cerr << "[Hardware] ERROR: No hay 'impresoraPuerto_' cargado." << std::endl;
        return false;
    }

    std::cout << "[Hardware] (STUB) Conectando a impresora en: "
              << this->impresoraPuerto_ << std::endl;

    // TODO: Implementar apertura real del puerto serie/USB

    std::cout << "[Hardware] (STUB) Puerto de impresora abierto con éxito." << std::endl;
    return true;
}

// ========================================================
// 5. Funciones públicas de operación
// ========================================================

// Capturar huella -> template (1:N)
bool HardwareManager::CapturarHuella(std::vector<uint8_t>& out_tpl) {
    if (!dispositivosListos_ || !dev_handle_) {
        std::cerr << "[Hardware] ERROR: CapturarHuella llamado antes de Init." << std::endl;
        return false;
    }

    out_tpl.resize(ZKFP_TEMPLATE_SIZE);
    unsigned int tpl_size = ZKFP_TEMPLATE_SIZE;

    int rc = fp_capture(dev_handle_.get(), out_tpl.data(), &tpl_size);
    if (rc != 0) {
        std::cerr << "[Hardware] Captura de huella falló. (Error SDK: " << rc << ")"
                  << std::endl;
        return false;
    }

    out_tpl.resize(tpl_size);
    return true;
}

// Identificar huella contra el caché del SDK (N templates en RAM)
// Nota: con el wrapper actual de fp_db_identify no podemos reconstruir el RUN real
// a partir del fid (hash). out_run va a quedar vacío; puedes cambiar esto más adelante
// si mantienes tu propia tabla fid->RUN.
std::string HardwareManager::IdentificarHuellaEnCache(
    const std::vector<uint8_t>& tpl_capturada
) {
    if (!dispositivosListos_ || !dev_handle_) {
        std::cerr << "[Hardware] ERROR: IdentificarHuella llamado antes de Init." << std::endl;
        return "";
    }

    std::string run_encontrado;
    unsigned int score = 0;

    int rc = fp_db_identify(
        dev_handle_.get(),
        tpl_capturada.data(),
        static_cast<unsigned int>(tpl_capturada.size()),
        run_encontrado,
        &score
    );

    if (rc == 0) {
        std::cout << "[Hardware] Huella identificada. RUN: "
                  << (run_encontrado.empty() ? "(desconocido)" : run_encontrado)
                  << ", Score: " << score << std::endl;
        return run_encontrado;  // hoy probablemente será ""; lo puedes mejorar después
    } else {
        std::cout << "[Hardware] Huella NO identificada (rc=" << rc
                  << ", score=" << score << ")." << std::endl;
        return "";
    }
}

// Comparación 1:1 entre dos templates (verificación directa)
bool HardwareManager::VerificarCoincidencia(
    const std::vector<uint8_t>& tpl1,
    const std::vector<uint8_t>& tpl2
) {
    if (!dispositivosListos_ || !dev_handle_) {
        std::cerr << "[Hardware] ERROR: VerificarCoincidencia llamado antes de Init."
                  << std::endl;
        return false;
    }

    int score = fp_match(
        dev_handle_.get(),
        tpl1.data(), static_cast<unsigned int>(tpl1.size()),
        tpl2.data(), static_cast<unsigned int>(tpl2.size())
    );

    if (score >= COINCIDENCIA) {
        std::cout << "[Hardware] Verificación 1:1 exitosa (Score: "
                  << score << ")" << std::endl;
        return true;
    }

    std::cout << "[Hardware] Verificación 1:1 fallida (Score: "
              << score << ")" << std::endl;
    return false;
}

// ========================================================
// 6. Stubs de impresión y semáforo
// ========================================================

bool HardwareManager::ImprimirTicket(
    const PerfilEstudiante& /*perfil*/,
    const RegistroRacion&   /*racion*/
) {
    if (!dispositivosListos_) return false;

    std::cout << "[Hardware] (STUB) Imprimiendo ticket en "
              << this->impresoraPuerto_ << "..." << std::endl;

    // TODO: Implementación real de impresión

    return true;
}

void HardwareManager::ActivarSemaforoGui(
    EstadoSemaforo estado,
    const std::string& mensaje
) {
    switch (estado) {
        case EstadoSemaforo::APROBADO_PAE:
        case EstadoSemaforo::APROBADO_NORMAL:
            std::cout << "[Hardware] (GUI-STUB) LUZ VERDE: "
                      << mensaje << std::endl;
            break;

        case EstadoSemaforo::RECHAZADO_DOBLE:
        case EstadoSemaforo::RECHAZADO_NO_PAE:
        case EstadoSemaforo::RECHAZADO_NO_EXISTE:
            std::cerr << "[Hardware] (GUI-STUB) LUZ ROJA: "
                      << mensaje << std::endl;
            break;
    }
}
