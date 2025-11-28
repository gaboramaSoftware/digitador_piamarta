// h/DB_Backend.hpp

#pragma once

#include "DB_models.hpp" // (Paso 1)
#include "Root_Admin.hpp"
#include "sqlite3.h" // El handle de la librería C


#include <map>
#include <mutex>
#include <optional> // Para tu API de configuración
#include <string>
#include <vector>


// Fwd-declare
struct sqlite3;

class DB_Backend {
public:
  // --- 1. Inicialización y RAII ---

  // Constructor: Solo guarda la ruta, no abre la DB.
  DB_Backend(const std::string &db_path);

  // Destructor: Cierra el handle de la DB si está abierto.
  ~DB_Backend();

  // Abre la conexión y crea/valida el esquema de tablas.
  // Esta es la primera función que se debe llamar.
  bool Inicializar_DB();

  // --- 2. API de Configuración (Tu diseño de Fila Única) ---

  // Obtiene la configuración (ID de terminal y modo)
  std::optional<Configuracion> Obtener_Configuracion();

  // Guarda/Actualiza la configuración
  bool Guardar_Configuracion(const Configuracion &config);

  // --- 3. API de Hardware (El caché 1:N) ---

  // Carga TODOS los templates de estudiantes (Rol=3) en un mapa.
  // Se llama 1 vez al iniciar el HardwareManager.
  bool Obtener_Todos_Templates(
      std::map<std::string, std::vector<uint8_t>> &out_templates);

  // --- 4. API de Lógica de Negocio (El Semáforo) ---

  // Obtiene el perfil completo (JOIN) de un usuario por su RUN.
  // Retorna el Rol (0 si no existe) y llena el PerfilEstudiante.
  RolUsuario Obtener_Perfil_Por_Run(const std::string &run_id,
                                    PerfilEstudiante &out_perfil);

  // Verifica si ya existe un registro para (estudiante, fecha, tipo).
  // (La lógica del RF2 - No Doble Ración)
  bool Verificar_Racion_Doble(const std::string &run_id,
                              const std::string &fecha_iso, // "YYYY-MM-DD"
                              TipoRacion racion);

  // Guarda un nuevo registro de ración en la DB.
  // El 'estado_registro' por defecto es PENDIENTE.
  bool Guardar_Registro_Racion(const RegistroRacion &registro);

  // --- 5. API de Sincronización (El Servicio de Sync) ---

  // Obtiene todos los registros con estado = PENDIENTE.
  std::vector<RegistroRacion> Obtener_Registros_Pendientes();

  // Marca una lista de registros (por ID) como SINCRONIZADO.
  bool Marcar_Registros_Sincronizados(
      const std::vector<std::int64_t> &ids_registros);

  // --- 6. API de Administración (Enrolamiento) ---

  // Enrola un nuevo estudiante (Inserta en Usuarios y DetallesEstudiante)
  bool Enrolar_Estudiante_Completo(const RequestEnrolarUsuario &request);

  // Elimina un usuario y sus detalles (ON DELETE CASCADE)
  bool Eliminar_Usuario(const std::string &run_id);

  //-- borrar todos los datos --
  bool borrarTodo();

private:
  // --- Miembros Privados ---

  // El handle (puntero) a la conexión de SQLite
  sqlite3 *db_handle_ = nullptr;

  // La ruta al archivo .db
  std::string db_path_;

  // El Mutex para hacer la clase "thread-safe"
  // CUALQUIER función pública que toque 'db_handle_' debe lockearlo.
  std::mutex db_mutex_;

  // --- Helper Privado ---

  // Helper para ejecutar SQL simple (como CREATE TABLE)
  bool _ejecutar_sql_script(const char *sql);

  // --- Evitar Copias ---
  DB_Backend(const DB_Backend &) = delete;
  DB_Backend &operator=(const DB_Backend &) = delete;
};