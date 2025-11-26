#pragma once

#include <string>
#include <vector>

#include "db_models.hpp"

// Declaraciones anticipadas
class Hardware_Manager; 
class DB_Access_Layer;


// --- CLASE BUSINESS_LOGIC ---
class Business_Logic {
public:
    // --- Constructor (Inyección de Dependencia) ---
    Business_Logic(Hardware_Manager* hw_manager, DB_Access_Layer* db_manager);

    // --- Destructor ---
    ~Business_Logic() = default; 

    // --- Flujo Principal de Operación (El Semáforo) ---
    void Iniciar_Bucle_Principal();


    // --- Funciones de Enrolamiento Asistido ---
    bool Manejar_Enrolamiento_Asistido(
        const std::string& run_operador,
        const EnrolamientoRequest& request 
    );

private:
    // --- Punteros a los "Trabajadores" ---
    Hardware_Manager* hw_manager_; 
    DB_Access_Layer* db_manager_;

    // --- Métodos Privados (Helpers de Lógica) ---

    void _procesar_huella_capturada();

    RolUsuario _identificar_usuario(
        const std::vector<uint8_t>& tpl, 
        PerfilEstudiante& out_perfil 
    );

    EstadoSemaforo _evaluar_elegibilidad_racion(const PerfilEstudiante& perfil);

    void _finalizar_transaccion(
        const PerfilEstudiante& perfil, 
        EstadoSemaforo estado 
    );

    void _obtener_contexto_actual(
        std::string& out_fecha_iso, 
        TipoRacion& out_tipo_racion 
    );
};