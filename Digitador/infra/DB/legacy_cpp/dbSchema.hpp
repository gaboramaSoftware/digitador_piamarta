// archivo para construir la base de datos

#pragma once
#include <string>

namespace Schema {
// Usamos R"(...)" para escribir SQL multilínea sin morir en el intento
const std::string TABLAS =
    R"(
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

const std::string SEED_ADMIN = "INSERT OR IGNORE INTO Usuarios (run_id, "
                               "nombre_completo, id_rol, template_huella) "
                               "VALUES (?, ?, ?, ?);";
} // namespace Schema