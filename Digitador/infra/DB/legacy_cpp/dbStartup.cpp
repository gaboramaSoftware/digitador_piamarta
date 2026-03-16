// funcion que se encarga de construir e inicializar la base de datos

#include "DB/dbInit.hpp"
#include "Server/Utilities/ServerStartup.hpp"
#include <iostream>
#include <stdexcept> // Para std::runtime_error
#include <stdlib.h>
#include <vector>

// 1. INICIALIZAR DB (esto lo va a hacer ServerStartup al iniciar el sistema)
bool DB_init::Inicializar_DB(const std::string &db_path, sqlite3 *&db_handle) {
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

// ok, pero ahora como le pido al ServerStartup que inicialice?