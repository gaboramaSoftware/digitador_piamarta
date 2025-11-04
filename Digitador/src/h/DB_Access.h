// DB_Access_Layer.h  (C puro)
#ifndef DB_ACCESS_LAYER_H_C
#define DB_ACCESS_LAYER_H_C

#include <stdbool.h>
#include <sqlite3.h>
#include "db_models.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char    *db_path;
    sqlite3 *db_handle;
} DB_Access_Layer_C;

// ciclo de vida
int  DBAL_init(DB_Access_Layer_C *self, const char *db_path);
void DBAL_destroy(DB_Access_Layer_C *self);

// conexión/esquema
bool DBAL_Abrir_Conexion(DB_Access_Layer_C *self);
void DBAL_Cerrar_Conexion(DB_Access_Layer_C *self);
bool DBAL_Inicializar_DB(DB_Access_Layer_C *self); // crea tablas si no existen

// operaciones estudiantes
bool DBAL_Guardar_Template(DB_Access_Layer_C *self, const Estudiante_C *estu); // upsert
bool DBAL_Obtener_Template_Por_ID(DB_Access_Layer_C *self,
                                  const char *run_id,
                                  /*out*/ Template_Huella_C *tpl_out,
                                  /*out*/ int *tpl_copied_size,
                                  /*in/out*/ uint8_t *buffer, int buffer_len);

// operaciones raciones
bool DBAL_Buscar_Registro_Duplicado(DB_Access_Layer_C *self,
                                    const char *id_estudiante,
                                    const char *fecha_servicio,
                                    int tipo_racion,
                                    /*out*/ bool *existe);

bool DBAL_Guardar_Registro_Racion(DB_Access_Layer_C *self,
                                  const RegistroRacion_C *reg,
                                  /*out*/ long long *out_rowid);

// iteración de pendientes con callback (C no tiene vector)
typedef int (*DBAL_Pendiente_CB)(const RegistroRacion_C *row, void *user);
bool DBAL_Obtener_Registros_Pendientes(DB_Access_Layer_C *self,
                                       DBAL_Pendiente_CB cb,
                                       void *user);

// borrar por id_registro
bool DBAL_Eliminar_Registro_Por_ID(DB_Access_Layer_C *self, long long id_registro);

#ifdef __cplusplus
}
#endif
#endif // DB_ACCESS_LAYER_H_C
