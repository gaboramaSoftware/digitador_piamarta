#pragma once

#include <cstdint>   // std::uint8_t
#include <memory>    // std::unique_ptr
#include <string>
#include <vector>

#include "DB_models.hpp"  // PerfilEstudiante, RegistroRacion, EstadoSemaforo

// --- DECLARACIONES ANTICIPADAS ---

class DB_Backend;   // Clase que maneja la DB
struct fp_device;   // Tipo opaco del SDK ZKTeco

// --- RAII PARA EL DISPOSITIVO ZKTECO ---

struct DevCloser {
    void operator()(fp_device* d) const;  // Implementado en HardwareManager.cpp (fp_close)
};

using DevPtr = std::unique_ptr<fp_device, DevCloser>;

// --- CLASE HARDWAREMANAGER ---

class HardwareManager {
public:
    explicit HardwareManager(DB_Backend* db_manager);
    ~HardwareManager();

    // Inicializa todo el hardware (sensor + impresora)
    bool Inicializar_Dispositivos();

    // Biometría
    bool CapturarHuella(std::vector<std::uint8_t>& out_tpl);

    std::string IdentificarHuellaEnCache(
        const std::vector<std::uint8_t>& tpl_capturada
    );

    bool VerificarCoincidencia(
        const std::vector<std::uint8_t>& tpl1,
        const std::vector<std::uint8_t>& tpl2
    );

    // Salida (ticket + “semáforo” lógico para GUI)
    bool ImprimirTicket(
        const PerfilEstudiante& perfil,
        const RegistroRacion&   racion
    );

    void ActivarSemaforoGui(
        EstadoSemaforo estado,
        const std::string& mensaje
    );

private:
    // Dependencias
    DB_Backend* db_manager_ = nullptr;
    DevPtr      dev_handle_;   // Manejador del dispositivo ZKTeco (RAII)

    // Configuración cargada desde DB
    std::string idTerminal_;       // Ej: "NUC-01"
    std::string impresoraPuerto_;  // Ej: "COM3" o "/dev/ttyUSB0"
    bool        dispositivosListos_ = false;

    // Helpers internos
    bool _cargarPersistencia();  // Lee ConfiguracionGlobal
    bool _abrirZKTeco();         // Abre lector + carga caché de huellas
    bool _abrirPuertos();        // Abre impresora (stub por ahora)
};
