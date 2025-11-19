#pragma once

#include <string>
#include <vector>
#include <memory>         // Para std::unique_ptr
#include "Estructuras.hpp" // Para Template_Huella, PerfilEstudiante, etc.

// --- DECLARACIONES ANTICIPADAS ---

// 1. Declaración anticipada de la clase de la que dependemos
class DB_Backend;

// 2. Declaración anticipada del struct C del SDK de ZKTeco
struct fp_device;

// --- GESTOR DE RECURSOS (RAII) PARA EL SDK DE C ---
struct DevCloser {
    void operator()(fp_device* d) const; // La implementación (fp_close) va en el .cpp
};

// Alias "Modo Pro" para el puntero del dispositivo
using DevPtr = std::unique_ptr<fp_device, DevCloser>;


// --- CLASE HARDWARE MANAGER ---

class HardwareManager {
public:
    // --- Constructor / Destructor (RAII) ---
    HardwareManager(DB_Backend* db_manager); ~HardwareManager();

    // --- Inicialización Híbrida (Tu Requerimiento) ---
    bool Inicializar_Dispositivos();
    // --- Funciones de Operación (Biometría) ---
    bool CapturarHuellaConReset(std::vector<uint8_t>& out_tpl);
    // verificar plantilla
    bool VerificarCoincidencia(const std::vector<uint8_t>& tpl1, const std::vector<uint8_t>& tpl2);


    // --- Funciones de Operación (Hardware de Salida) ---
    bool ImprimirTicket(const PerfilEstudiante& perfil, const RegistroRacion& racion);
    // activar semaforo
    void ActivarSemáforoGui(ResultadoSemaforo estado, const std::string& mensaje);

private:
    // --- Punteros de Dependencia y Hardware ---
    
    // El "teléfono" al Archivista (para cargar config)
    DB_Backend* db_manager_; 
    // El puntero al SDK de ZKTeco (gestionado por RAII)
    DevPtr dev_handle_;
    // --- Configuración Persistente/Descubierta ---
    std::string idTerminal_;      // Ej: "NUC-01"
    std::string impresoraPuerto_; // Ej: "COM3" o "/dev/ttyUSB0"
    bool dispositivosListos_ = false;

    // --- Métodos Privados (Helpers) ---
    
    // Intenta leer la config desde la DB (Paso 1)
    bool _cargarPersistencia();    
    // Intenta escanear puertos USB (Paso 2)
    bool _autodescubrirDispositivos();
    // Llama a fp_open() y configura el SDK
    bool _abrirZKTeco();
    // Se conecta al puerto de la impresora
    bool _abrirPuertos();
};