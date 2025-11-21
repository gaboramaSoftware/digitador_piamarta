/*
 * h/DB_models.hpp
 * * Definición de todas las estructuras de datos, DTOs y enums
 * que se usan en el proyecto. Este archivo no depende de nada,
 * solo de <string>, <vector>, etc.
 */

#pragma once

#include <cstdint>  // Para std::int64_t y std::uint8_t
#include <string>
#include <vector>

// --- ENUMS DE LÓGICA ---

// Define el rol de un usuario en el sistema
enum class RolUsuario : int {
    // 0 es un rol inválido (para retornos de error)
    Invalido = 0, 
    Administrador = 1,
    Operador = 2,
    Estudiante = 3,
};

// Define el tipo de servicio (desayuno, almuerzo, etc.)
enum class TipoRacion : int {
    RACION_NO_APLICA = 0, // Configuración por defecto o error
    Desayuno = 1,
    Almuerzo = 2,
};

// Define el resultado de la validación del semáforo (para la UI)
// Es más descriptivo que un simple bool.
enum class EstadoSemaforo : int {
    APROBADO_PAE = 1,      // Luz Verde (con marca PAE)
    APROBADO_NORMAL = 2,   // Luz Verde (sin marca PAE)
    RECHAZADO_DOBLE = 3,   // Luz Roja (ya comió)
    RECHAZADO_NO_PAE = 4,  // Luz Roja (no es PAE, modo Desayuno/Once)
    RECHAZADO_NO_EXISTE = 5, // Luz Roja (no encontrado)
};

// Define el estado de sincronización de un registro
enum class EstadoRegistro : int {
    PENDIENTE = 0,     // Creado localmente, no enviado al servidor
    SINCRONIZADO = 1,  // Ya enviado y confirmado por el servidor
};


// --- ESTRUCTURAS DE TABLAS (SQL) ---

// Corresponde a la tabla 'Usuarios'
struct Usuario {
    std::string run_id; // PK
    char dv;
    std::string nombre_completo;
    RolUsuario id_rol = RolUsuario::Estudiante;
    std::string password_hash; // Para Operador/Admin
    std::vector<uint8_t> template_huella; // BLOB de la huella
}; 

// Corresponde a la tabla 'DetallesEstudiante'
struct DetalleEstudiante {
    std::string run_id;   // PK (y FK que apunta a Usuario.run_id)
    std::string curso;    // ej "1bA", "4mB"
    bool es_pae = false;  // Estado del beneficio (0 o 1)
}; 

// Corresponde a la tabla 'RegistrosRaciones'
struct RegistroRacion {
    std::int64_t id_registro = 0; // PK (AUTOINCREMENT)
    std::string id_estudiante;    // FK (run_id del Usuario)
    std::string fecha_servicio;   // ISO "YYYY-MM-DD"
    TipoRacion tipo_racion = TipoRacion::RACION_NO_APLICA;
    std::string id_terminal;      // ID del NUC (NUC-01, etc.)
    std::int64_t hora_evento = 0; // Hora exacta del evento (epoch ms)
    EstadoRegistro estado_registro = EstadoRegistro::PENDIENTE;
}; 

// Corresponde a la tabla 'ConfiguracionGlobal' (Tu diseño de Fila Única)
struct Configuracion {
    std::string id_terminal;
    TipoRacion tipo_racion;
    std::string puerto_impresora; //direccion de la impresora
};


// --- DTOs (Data Transfer Objects) ---
// Structs "Helper" que no son tablas, sino combinaciones lógicas

// Un perfil completo de estudiante (resultado de un JOIN)
struct PerfilEstudiante {
    // --- De Usuario ---
    std::string run_id;
    char dv;
    std::string nombre_completo;
    RolUsuario id_rol;
    std::vector<uint8_t> template_huella;
    
    // --- De DetalleEstudiante ---
    std::string curso;
    bool es_pae;
};

// Un DTO para la Business_Logic al enrolar un nuevo usuario
struct RequestEnrolarUsuario {
    std::string run_nuevo;
    char dv_nuevo;
    std::string nombre_nuevo;
    std::string curso_nuevo; // Corregido (era std:: curso_nuevo)
    bool es_pae;
    std::vector<uint8_t> template_huella;
};