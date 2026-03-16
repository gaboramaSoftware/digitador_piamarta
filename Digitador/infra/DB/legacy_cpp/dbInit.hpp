// Funcion que se encarga de iniciar y destruir la instancia de la base de datos

#pragma once
#include "sqlite3.h"
#include <mutex>
#include <optional>
#include <string>

class DB_init {
public:
  // devuelve true si salio todo bien
  static bool Inicializar_DB(const std::string &db_path, sqlite3 *&db_handle);

  static bool Inicializar_DB(const std::string &db_path);

private:
  // Helper para ejecutar SQL simple (como CREATE TABLE)
  static bool _ejecutar_sql_script(sqlite3 *&db_handle, const char *sql);
  static bool insertarAdminBase(sqlite3 *db);
};