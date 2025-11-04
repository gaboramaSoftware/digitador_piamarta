// DB_Access.cpp
#include "DB_Access.h"      // si tu header está en h/, usa: #include "h/DB_Access.h"
#include <iostream>
#include <cstdio>
#include <sqlite3.h>
#include <string>

// Constructor: Inicializa los atributos
DB_Access_Layer::DB_Access_Layer(const std::string& db_path)
    : db_path_(db_path), db_handle_(nullptr) {
    // El constructor solo guarda la ruta; no abre la DB todavía.
}

// Destructor: Limpia y cierra la conexión
DB_Access_Layer::~DB_Access_Layer() {
    if (db_handle_) {
        sqlite3_close(db_handle_);
        db_handle_ = nullptr; // FIX: evita puntero colgante
        std::cout << "Conexión DB cerrada." << std::endl;
    }
}

// --- Inicializar DB / crear schema si no existe ---
bool DB_Access_Layer::Inicializar_DB() {
    // 1) Abrir o crear archivo de base de datos
    int rc = sqlite3_open_v2(
        db_path_.c_str(),
        &db_handle_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );

    if (rc != SQLITE_OK) {
        // FIX: si open falla, db_handle_ puede ser nullptr.
        const char* emsg = db_handle_ ? sqlite3_errmsg(db_handle_) : "sqlite3_open_v2 failed";
        std::cerr << "Error al abrir/crear DB: " << emsg << std::endl;
        return false;
    }

    // 2) Definir el esquema de tablas
    const char* sql_schema =
        "CREATE TABLE IF NOT EXISTS Estudiantes ("
        "    run_id TEXT PRIMARY KEY NOT NULL,"
        "    nombres TEXT NOT NULL,"
        "    curso TEXT,"
        "    template_data BLOB NOT NULL,"
        "    id_rol INTEGER NOT NULL,"
        "    es_elegible_pae BOOLEAN DEFAULT 1"
        ");"
        "CREATE TABLE IF NOT EXISTS RegistrosRaciones ("
        "    id_registro INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    id_estudiante TEXT NOT NULL,"
        "    id_terminal TEXT NOT NULL,"
        "    tipo_racion INTEGER NOT NULL,"
        "    fecha_servicio TEXT NOT NULL,"
        "    timestamp_evento INTEGER NOT NULL,"
        "    estado_registro INTEGER NOT NULL,"
        "    FOREIGN KEY (id_estudiante) REFERENCES Estudiantes (run_id),"
        "    UNIQUE (id_estudiante, fecha_servicio, tipo_racion)"
        ");";

    // 3) Ejecutar el SQL para crear las tablas
    char* zErrMsg = nullptr;
    rc = sqlite3_exec(db_handle_, sql_schema, nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error en SQL al crear schema: " << (zErrMsg ? zErrMsg : "(sin mensaje)") << std::endl;
        if (zErrMsg) sqlite3_free(zErrMsg);
        return false;
    }

    std::cout << "Base de datos inicializada y tablas creadas en: " << db_path_ << std::endl;
    return true;
}
