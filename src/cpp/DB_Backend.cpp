// src/cpp/DB_Backend.cpp

#include "DB_Backend.hpp"
#include <iostream>
#include <stdexcept> // Para std::runtime_error
#include <stdlib.h>

//  Helpers de mapeo enum <-> int (locales a este .cpp)
static std::optional<TipoRacion> to_tipo_racion(int v) {
  switch (v) {
  case 0:
    return TipoRacion::RACION_NO_APLICA;
  case 1:
    return TipoRacion::Desayuno;
  case 2:
    return TipoRacion::Almuerzo;
  default:
    return std::nullopt;
  }
}
static int to_int(TipoRacion t) { return static_cast<int>(t); }

static std::optional<RolUsuario> to_rol_usuario(int v) {
  switch (v) {
  case 1:
    return RolUsuario::Administrador;
  case 2:
    return RolUsuario::Operador;
  case 3:
    return RolUsuario::Estudiante;
  default:
    return RolUsuario::Invalido;
  }
}

// ===== 1. Inicialización y RAII =====

// CONSTRUCTOR
DB_Backend::DB_Backend(const std::string &db_path)
    : db_handle_(nullptr), db_path_(db_path) {}

// DESTRUCTOR
DB_Backend::~DB_Backend() {
  if (db_handle_) {
    sqlite3_close(db_handle_);
    std::cout << "[DB_Backend] Conexión DB cerrada." << std::endl;
  }
}

// INICIALIZAR DB
bool DB_Backend::Inicializar_DB() {
  int rc = sqlite3_open_v2(db_path_.c_str(), &db_handle_,
                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                               SQLITE_OPEN_FULLMUTEX,
                           nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR: No se pudo abrir/crear DB en: "
              << db_path_ << " - " << sqlite3_errmsg(db_handle_) << "\n";
    return false;
  }

  // Habilitar soporte de Foreign Keys
  if (!_ejecutar_sql_script("PRAGMA foreign_keys = ON;")) {
    std::cerr << "[DB_Backend] WARN: No se pudo habilitar foreign_keys.\n";
  }

  // Esquema
  const char *sql_schema =
      // Usuarios
      "CREATE TABLE IF NOT EXISTS Usuarios ("
      "    run_id TEXT PRIMARY KEY NOT NULL,"
      "    dv CHAR(1) NOT NULL,"
      "    nombre_completo TEXT NOT NULL,"
      "    id_rol INTEGER NOT NULL DEFAULT 3," // 3=Estudiante
      "    password_hash TEXT,"
      "    template_huella BLOB NOT NULL"
      ");"
      // DetallesEstudiante
      "CREATE TABLE IF NOT EXISTS DetallesEstudiante ("
      "    run_id TEXT PRIMARY KEY NOT NULL,"
      "    curso TEXT,"
      "    FOREIGN KEY (run_id) REFERENCES Usuarios (run_id) ON DELETE CASCADE"
      ");"
      // RegistrosRaciones (PLURAL)
      "CREATE TABLE IF NOT EXISTS RegistrosRaciones ("
      "    id_registro INTEGER PRIMARY KEY AUTOINCREMENT,"
      "    id_estudiante TEXT NOT NULL,"
      "    fecha_servicio TEXT NOT NULL," // "YYYY-MM-DD"
      "    tipo_racion INTEGER NOT NULL," // 0, 1, 2
      "    id_terminal TEXT NOT NULL,"
      "    hora_evento INTEGER NOT NULL,"     // epoch ms
      "    estado_registro INTEGER NOT NULL," // 0=PENDIENTE, 1=SINCRONIZADO
      "    FOREIGN KEY (id_estudiante) REFERENCES Usuarios (run_id),"
      "    UNIQUE (id_estudiante, fecha_servicio, tipo_racion)" // (RF2)
      ");"
      // Configuración global
      "CREATE TABLE IF NOT EXISTS ConfiguracionGlobal ("
      "    id_unica INTEGER PRIMARY KEY CHECK (id_unica = 1),"
      "    id_terminal TEXT NOT NULL,"
      "    tipo_racion INTEGER NOT NULL CHECK (tipo_racion IN (0,1,2)),"
      "    puerto_impresora TEXT NOT NULL DEFAULT 'COM3'"
      ");";

  if (!_ejecutar_sql_script(sql_schema)) {
    std::cerr << "[DB_Backend] ERROR: Falló la inicialización del esquema.\n";
    sqlite3_close(db_handle_);
    db_handle_ = nullptr;
    return false;
  }

  const char *sql_admin =
      "INSERT OR IGNORE INTO Usuarios "
      "(run_id, dv, nombre_completo, id_rol, password_hash, template_huella) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";

  sqlite3_stmt *stmt_admin = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql_admin, -1, &stmt_admin, nullptr) ==
      SQLITE_OK) {

    sqlite3_bind_text(stmt_admin, 1, Root_Admin::DEFAULT_RUN.c_str(), -1,
                      SQLITE_STATIC);
    sqlite3_bind_text(stmt_admin, 2, Root_Admin::DEFAULT_DV.c_str(), -1,
                      SQLITE_STATIC);
    sqlite3_bind_text(stmt_admin, 3, Root_Admin::DEFAULT_NAME.c_str(), -1,
                      SQLITE_STATIC);
    sqlite3_bind_int(stmt_admin, 4, Root_Admin::DEFAULT_ROLE);
    sqlite3_bind_text(stmt_admin, 5, Root_Admin::DEFAULT_PASS_HASH.c_str(), -1,
                      SQLITE_STATIC);
    sqlite3_bind_zeroblob(stmt_admin, 6, 0);

    if (sqlite3_step(stmt_admin) != SQLITE_DONE) {
      std::cerr
          << "[DB_Backend] WARN: No se pudo insertar el Admin por defecto.\n";
    }
    sqlite3_finalize(stmt_admin);
  } else {
    std::cerr << "[DB_Backend] ERROR: Falló prepare del Admin Seed.\n";
  }

  std::cout << "[DB_Backend]: Base de datos inicializada en: " << db_path_
            << "\n";
  return true;
}

//=== 2. API de Configuración ===

std::optional<Configuracion> DB_Backend::Obtener_Configuracion() {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char *sql = "SELECT id_terminal, tipo_racion, puerto_impresora FROM "
                    "ConfiguracionGlobal WHERE id_unica = 1;";

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR prepare Obtener_Configuracion: "
              << sqlite3_errmsg(db_handle_) << "\n";
    return std::nullopt;
  }

  std::optional<Configuracion> out;
  int rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char *t0 = sqlite3_column_text(stmt, 0);
    std::string id_terminal = t0 ? reinterpret_cast<const char *>(t0) : "";
    int v_tipo = sqlite3_column_int(stmt, 1);

    const unsigned char *t2 = sqlite3_column_text(stmt, 2);
    std::string puerto_impresora =
        t2 ? reinterpret_cast<const char *>(t2) : "COM3";
    auto maybe = to_tipo_racion(v_tipo);

    if (maybe) {
      out = Configuracion{id_terminal, *maybe, puerto_impresora};
    } else {
      out = Configuracion{id_terminal, TipoRacion::RACION_NO_APLICA,
                          puerto_impresora};
    }
  }

  sqlite3_finalize(stmt);
  return out;
}

bool DB_Backend::Guardar_Configuracion(const Configuracion &config) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char *sql =
      "UPDATE ConfiguracionGlobal "
      "SET id_terminal = ?1, tipo_racion = ?2, puerto_impresora = ?3 "
      "WHERE id_unica = 1;";

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR prepare Guardar_Configuracion: "
              << sqlite3_errmsg(db_handle_) << "\n";
    return false;
  }

  sqlite3_bind_text(stmt, 1, config.id_terminal.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, to_int(config.tipo_racion));
  sqlite3_bind_text(stmt, 3, config.puerto_impresora.c_str(), -1,
                    SQLITE_TRANSIENT);

  int rc = sqlite3_step(stmt);
  bool exito = (rc == SQLITE_DONE);

  sqlite3_finalize(stmt);
  return exito;
}

// --- 3. API de Hardware (El caché 1:N) ---
bool DB_Backend::Obtener_Todos_Templates(
    std::map<std::string, std::vector<uint8_t>> &out_templates) {
  std::lock_guard<std::mutex> lock(db_mutex_);
  out_templates.clear();

  const char *sql = "SELECT run_id, template_huella FROM Usuarios WHERE "
                    "template_huella IS NOT NULL;";

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR prepare Obtener_Todos_Templates: "
              << sqlite3_errmsg(db_handle_) << "\n";
    return false;
  }

  int count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *txt_run = sqlite3_column_text(stmt, 0);
    if (!txt_run)
      continue;

    std::string run = reinterpret_cast<const char *>(txt_run);

    const void *blob = sqlite3_column_blob(stmt, 1);
    int size = sqlite3_column_bytes(stmt, 1);

    if (blob && size > 0) {
      std::vector<uint8_t> temp(static_cast<const uint8_t *>(blob),
                                static_cast<const uint8_t *>(blob) + size);
      out_templates[run] = temp;
      count++;
    }
  }

  sqlite3_finalize(stmt);
  std::cout << "[DB_Backend] Se cargaron " << count
            << " templates en memoria.\n";
  return true;
}

// --- 4. API de Lógica de Negocio (El Semáforo) ---

RolUsuario DB_Backend::Obtener_Perfil_Por_Run(const std::string &run_id,
                                              PerfilEstudiante &out_perfil) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char *sql_join =
      "SELECT "
      "U.dv, U.nombre_completo, U.id_rol, U.template_huella, "
      "D.curso "
      "FROM Usuarios U "
      "LEFT JOIN DetallesEstudiante D ON U.run_id = D.run_id "
      "WHERE U.run_id = ?1;";

  sqlite3_stmt *stmt = nullptr;
  RolUsuario rol_encontrado = RolUsuario::Invalido;

  try {
    if (sqlite3_prepare_v2(db_handle_, sql_join, -1, &stmt, nullptr) !=
        SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_handle_));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
      out_perfil.run_id = run_id;
      out_perfil.dv = sqlite3_column_text(stmt, 0)[0];
      out_perfil.nombre_completo =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

      int rol_int = sqlite3_column_int(stmt, 2);
      rol_encontrado = to_rol_usuario(rol_int).value_or(RolUsuario::Invalido);
      out_perfil.id_rol = rol_encontrado;

      const void *blob_data = sqlite3_column_blob(stmt, 3);
      int blob_size = sqlite3_column_bytes(stmt, 3);

      if (blob_data && blob_size > 0) {
        out_perfil.template_huella.assign(
            static_cast<const uint8_t *>(blob_data),
            static_cast<const uint8_t *>(blob_data) + blob_size);
      } else {
        out_perfil.template_huella.clear();
      }

      const unsigned char *curso_txt = sqlite3_column_text(stmt, 4);
      out_perfil.curso =
          curso_txt ? reinterpret_cast<const char *>(curso_txt) : "";
    } else if (rc != SQLITE_DONE) {
      throw std::runtime_error(sqlite3_errmsg(db_handle_));
    }

  } catch (const std::exception &e) {
    std::cerr << "[DB_Backend] ERROR al obtener perfil: " << e.what() << "\n";
    rol_encontrado = RolUsuario::Invalido;
  }

  if (stmt)
    sqlite3_finalize(stmt);

  return rol_encontrado;
}

bool DB_Backend::Verificar_Racion_Doble(const std::string &run_id,
                                        const std::string &fecha_iso,
                                        TipoRacion racion) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char *sql = "SELECT EXISTS("
                    "   SELECT 1 FROM RegistrosRaciones"
                    "   WHERE id_estudiante = ?1"
                    "     AND fecha_servicio = ?2"
                    "     AND tipo_racion = ?3"
                    ");";

  sqlite3_stmt *stmt = nullptr;
  bool is_duplicate = false;

  try {
    int rc;
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
      throw std::runtime_error("Preparar Verificar_Racion_Doble falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }

    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, fecha_iso.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, to_int(racion));

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
      is_duplicate = (sqlite3_column_int(stmt, 0) == 1);
    } else if (rc != SQLITE_DONE) {
      throw std::runtime_error("Ejecutar Verificar_Racion_Doble falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }

  } catch (const std::exception &e) {
    std::cerr << "[DB_Backend] ERROR en Verificar_Racion_Doble: " << e.what()
              << "\n";

    is_duplicate = false;
  }

  if (stmt) {
    sqlite3_finalize(stmt);
  }
  return is_duplicate;
}

bool DB_Backend::Guardar_Registro_Racion(const RegistroRacion &registro) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  // CORREGIDO: 'RegistrosRaciones' (PLURAL)
  const char *sql = "INSERT INTO RegistrosRaciones "
                    "(id_estudiante, fecha_servicio, tipo_racion, id_terminal, "
                    "hora_evento, estado_registro)"
                    "VALUES (?1, ?2, ?3, ?4, ?5, ?6)";

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR prepare Guardar_Registro: "
              << sqlite3_errmsg(db_handle_) << "\n";

    return false;
  }

  sqlite3_bind_text(stmt, 1, registro.id_estudiante.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, registro.fecha_servicio.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, to_int(registro.tipo_racion));
  sqlite3_bind_text(stmt, 4, registro.id_terminal.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 5, registro.hora_evento);
  sqlite3_bind_int(stmt, 6, static_cast<int>(registro.estado_registro));

  int rc = sqlite3_step(stmt);
  bool exito = (rc == SQLITE_DONE);

  if (!exito) {
    std::cerr << "backend no guardo racion (posible duplicado): "
              << sqlite3_errmsg(db_handle_) << "\n";
  } else {
    std::cout << "[+ DB_BACKEND]: racion registrada para estudiante: "
              << registro.id_estudiante << "\n";
  }

  sqlite3_finalize(stmt);
  return exito;
}

// --- 5. API de Sincronización (El Servicio de Sync) ---

// AQUI ESTABA EL TODO - AHORA IMPLEMENTADO
std::vector<RegistroRacion> DB_Backend::Obtener_Registros_Pendientes() {
  std::lock_guard<std::mutex> lock(db_mutex_);
  std::vector<RegistroRacion> lista;

  const char *sql =
      "SELECT id_registro, id_estudiante, fecha_servicio, tipo_racion, "
      "id_terminal, hora_evento "
      "FROM RegistrosRaciones WHERE estado_registro = 0;"; // 0 = PENDIENTE

  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR prepare Obtener_Pendientes: "
              << sqlite3_errmsg(db_handle_) << "\n";

    return lista;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    RegistroRacion r;
    r.id_registro = sqlite3_column_int64(stmt, 0);

    const unsigned char *txt_est = sqlite3_column_text(stmt, 1);
    r.id_estudiante = txt_est ? reinterpret_cast<const char *>(txt_est) : "";

    const unsigned char *txt_fecha = sqlite3_column_text(stmt, 2);
    r.fecha_servicio =
        txt_fecha ? reinterpret_cast<const char *>(txt_fecha) : "";

    int tipo = sqlite3_column_int(stmt, 3);
    r.tipo_racion = to_tipo_racion(tipo).value_or(TipoRacion::RACION_NO_APLICA);

    const unsigned char *txt_term = sqlite3_column_text(stmt, 4);
    r.id_terminal = txt_term ? reinterpret_cast<const char *>(txt_term) : "";

    r.hora_evento = sqlite3_column_int64(stmt, 5);
    r.estado_registro = EstadoRegistro::PENDIENTE;

    lista.push_back(r);
  }

  sqlite3_finalize(stmt);
  return lista;
}

// AQUI ESTABA EL TODO - AHORA IMPLEMENTADO
bool DB_Backend::Marcar_Registros_Sincronizados(
    const std::vector<std::int64_t> &ids_registros) {
  std::lock_guard<std::mutex> lock(db_mutex_);
  if (ids_registros.empty())
    return true;

  _ejecutar_sql_script("BEGIN TRANSACTION;");

  const char *sql = "UPDATE RegistrosRaciones SET estado_registro = 1 WHERE "
                    "id_registro = ?1;";
  sqlite3_stmt *stmt = nullptr;

  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    _ejecutar_sql_script("ROLLBACK;");
    return false;
  }

  bool all_ok = true;
  for (auto id : ids_registros) {
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      all_ok = false;
      break;
    }
    sqlite3_reset(stmt);
  }

  sqlite3_finalize(stmt);

  if (all_ok) {
    _ejecutar_sql_script("COMMIT;");
    return true;
  } else {
    _ejecutar_sql_script("ROLLBACK;");
    return false;
  }
}

// --- 6. API de Administración (Enrolamiento) ---

bool DB_Backend::Enrolar_Estudiante_Completo(
    const RequestEnrolarUsuario &request) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  sqlite3_stmt *stmt_user = nullptr;
  sqlite3_stmt *stmt_details = nullptr;
  bool success = false;

  try {
    if (!_ejecutar_sql_script("BEGIN TRANSACTION;")) {
      throw std::runtime_error("No se pudo iniciar la transacción.");
    }

    // arreglar string de rut para los BIND
    std::string run_str = request.run_nuevo;

    // --- 1. INSERTAR USUARIO ---
    const char *sql_user = "INSERT INTO Usuarios (run_id, dv, nombre_completo, "
                           "id_rol, template_huella, password_hash) "
                           "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";

    if (sqlite3_prepare_v2(db_handle_, sql_user, -1, &stmt_user, nullptr) !=
        SQLITE_OK) {
      throw std::runtime_error("Preparar INSERT Usuarios falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }

    const char dv_array[2] = {request.dv_nuevo, '\0'};

    sqlite3_bind_text(stmt_user, 1, request.run_nuevo.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_user, 2, dv_array, 1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_user, 3, request.nombre_nuevo.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_user, 4, static_cast<int>(RolUsuario::Estudiante));

    sqlite3_bind_blob(stmt_user, 5, request.template_huella.data(),
                      static_cast<int>(request.template_huella.size()),
                      SQLITE_STATIC);

    sqlite3_bind_null(stmt_user, 6);

    if (sqlite3_step(stmt_user) != SQLITE_DONE) {
      throw std::runtime_error("Ejecutar INSERT Usuarios falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }
    sqlite3_finalize(stmt_user);
    stmt_user = nullptr;

    // --- 2. INSERTAR DETALLES ---
    const char *sql_details = "INSERT INTO DetallesEstudiante (run_id, curso) "
                              "VALUES (?1, ?2);";

    if (sqlite3_prepare_v2(db_handle_, sql_details, -1, &stmt_details,
                           nullptr) != SQLITE_OK) {
      throw std::runtime_error("Preparar INSERT Detalles falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }

    sqlite3_bind_text(stmt_details, 1, request.run_nuevo.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_details, 2, request.curso_nuevo.c_str(), -1,
                      SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_details) != SQLITE_DONE) {
      throw std::runtime_error("Ejecutar INSERT Detalles falló: " +
                               std::string(sqlite3_errmsg(db_handle_)));
    }
    sqlite3_finalize(stmt_details);
    stmt_details = nullptr;

    success = _ejecutar_sql_script("COMMIT;");
    if (!success) {
      throw std::runtime_error("COMMIT falló.");
    }

  } catch (const std::exception &e) {
    std::cerr << "[DB_Backend] ERROR fatal en Enrolamiento: " << e.what()
              << "\n";
    _ejecutar_sql_script("ROLLBACK;");
  }

  if (stmt_user)
    sqlite3_finalize(stmt_user);
  if (stmt_details)
    sqlite3_finalize(stmt_details);

  return success;
}

bool DB_Backend::Eliminar_Usuario(const std::string &run_id) {
  std::lock_guard<std::mutex> lock(db_mutex_);
  // Implementación opcional pero simple
  const char *sql = "DELETE FROM Usuarios WHERE run_id = ?1;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool DB_Backend::borrarTodo() {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char *sql_clear = "DELETE FROM RegistrosRaciones;"
                          "DELETE FROM DetallesEstudiante;"
                          "DELETE FROM Usuarios;";

  std::cout
      << "[DB_Backend][QA] Borrando todos los datos de tablas principales...\n";
  return _ejecutar_sql_script(sql_clear);
}

// ===== Helper Privado =====
bool DB_Backend::_ejecutar_sql_script(const char *sql) {
  char *zErrMsg = nullptr;
  int rc = sqlite3_exec(db_handle_, sql, nullptr, nullptr, &zErrMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "[DB_Backend] ERROR sqlite3_exec: "
              << (zErrMsg ? zErrMsg : "(sin mensaje)") << "\n";
    if (zErrMsg)
      sqlite3_free(zErrMsg);
    return false;
  }
  return true;
}