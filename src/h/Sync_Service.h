#pragma once

#include <string>
#include <vector>
#include <thread>         // Para correr en segundo plano
#include <mutex>          // Para seguridad entre hilos
#include <atomic>         // Para detener el hilo de forma segura
#include "db_models.hpp" // Para RegistroRacion, Usuario, etc.

// --- DECLARACIONES ANTICIPADAS ---
class DB_Backend; // Depende de la DB para obtener/guardar datos


// --- CLASE SYNC_SERVICE ---
class Sync_Service {
public:
    // --- Constructor (Inyección de Dependencia) ---

    //constructor
    Sync_Service(
        DB_Backend* db_manager,
        const std::string& server_ip,
        int server_port,
        int sync_interval_seconds
    );

    // --- Destructor ---
    ~Sync_Service();

    // --- Control del Hilo (Thread) ---
    void Iniciar_Servicio_En_Segundo_Plano();
    void Detener_Servicio();

private:
    // --- Punteros de Dependencia y Configuración ---
    DB_Backend* db_manager_; // El "Archivista"
    std::string server_ip_;
    int server_port_;
    int sync_interval_seconds_;

    // --- Control del Hilo (Threading) ---
    std::thread hilo_sincronizacion_; // El hilo de fondo
    std::atomic<bool> detener_hilo_{false}; // Bandera para detener el hilo
    std::mutex mtx_sync_; // Mutex para evitar sincronizaciones concurrentes

    // --- METODO PRIVADOS---

    // El bucle que corre en el hilo de fondo
    void _bucle_de_sincronizaci on();

    //sincronización real 
    bool _sincronizar_subida();

    //sincronizar bajada
    bool _sincronizar_bajada();

    //conectar al servidor
    int _conectar_al_servidor();
};