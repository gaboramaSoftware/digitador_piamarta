// db_models.h  (usable desde C y C++)
#ifndef DB_MODELS_H
#define DB_MODELS_H

#include <stdint.h>   
#include <stdbool.h>  
#ifdef __cplusplus
#include <string>
#include <vector>
extern "C" {
#endif

// Para C: plantilla binaria simple
typedef struct {
    const uint8_t *data;  
    int size;             
} Template_Huella_C;

// Estudiante (clave primaria: run_id)
typedef struct {
    const char *run_id;     // "12.345.678-9"
    const char *nombres;    // "Ana Pérez"
    const char *curso;      // "2°A"
    int id_rol;             // 1 = alumno, etc.
    bool es_elegible_pae;   // 1 si elegible
    Template_Huella_C tpl;  // blob de huella
} Estudiante_C;

// Registro de ración
typedef struct {
    int64_t id_registro;        
    const char *id_estudiante;  
    const char *id_terminal;    
    int tipo_racion;            // p.ej. 1=desayuno, 2=almuerzo
    const char *fecha_servicio; // "YYYY-MM-DD"
    int64_t timestamp_evento;   // epoch ms o s, según definas
    int estado_registro;        // p.ej. 0=pending, 1=ok, -1=error
} RegistroRacion_C;

#ifdef __cplusplus
} // extern "C"

// Conveniencias C++ con std::string / std::vector
struct Template_Huella {
    std::vector<uint8_t> bytes;
};

struct Estudiante {
    std::string run_id;
    std::string nombres;
    std::string curso;
    int id_rol{0};
    bool es_elegible_pae{true};
    Template_Huella tpl;
};

enum class Color { Rojo, Amarillo, Verde };

enum class Resultado_Semaforo {
    Rechazado_NoElegible,
    Rechazado_Duplicado,
    Aprobado_EntregarRacion
};

struct RegistroRacion {
    long long id_registro{0};
    std::string id_estudiante;
    std::string id_terminal;
    int tipo_racion{0};
    std::string fecha_servicio;
    long long timestamp_evento{0};
    int estado_registro{0};
};
#endif

#endif 
