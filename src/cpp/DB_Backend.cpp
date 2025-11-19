//DB_backend.cpp

#include "DB_Backend.hpp"
#include <iostream>
#include <stdexcept> // Para std::runtime_error

//  Helpers de mapeo enum <-> int (locales a este .cpp) 
static std::optional<TipoRacion> to_tipo_racion(int v) {
    switch (v) {
        case 0: return TipoRacion::RACION_NO_APLICA;
        case 1: return TipoRacion::Desayuno;
        case 2: return TipoRacion::Almuerzo;
        default: return std::nullopt;
    }
}
static int to_int(TipoRacion t) { return static_cast<int>(t); }

static std::optional<RolUsuario> to_rol_usuario(int v) {
    switch (v){
        case 1: return RolUsuario::Administrador;
        case 2: return RolUsuario::Operador;
        case 3: return RolUsuario::Estudiante;
        default: return RolUsuario::Invalido;
    }
}


// ===== 1. Inicialización y RAII =====

//CONSTRUCTOR DE BACKEND
DB_Backend::DB_Backend(const std::string& db_path)
    : db_path_(db_path), db_handle_(nullptr) {
        //debe estar vacio
}

//DESTRUCTOR DE BACKEND
DB_Backend::~DB_Backend() {
    if (db_handle_) {
        sqlite3_close(db_handle_);
        std::cout << "[DB_Backend] Conexión DB cerrada." << std::endl;
    }
}

//inicializar base de datos
bool DB_Backend::Inicializar_DB() {
    int rc = sqlite3_open_v2(
        db_path_.c_str(),
        &db_handle_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (rc != SQLITE_OK) {
        std::cerr << "[DB_Backend] ERROR: No se pudo abrir/crear DB en: " << db_path_
                  << " - " << sqlite3_errmsg(db_handle_) << "\n";
        return false;
    }

    // Habilitar soporte de Foreign Keys (¡Importante para ON DELETE CASCADE!)
    if (!_ejecutar_sql_script("PRAGMA foreign_keys = ON;")) {
        std::cerr << "[DB_Backend] WARN: No se pudo habilitar foreign_keys.\n";
        // No es fatal, pero es un aviso.
    }

    // Esquema (epoch en milisegundos; enums validados con CHECK)
    const char* sql_schema =
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
        "    es_pae INTEGER NOT NULL DEFAULT 0," // 0=false, 1=true
        "    FOREIGN KEY (run_id) REFERENCES Usuarios (run_id) ON DELETE CASCADE"
        ");"
        // RegistrosRaciones
        "CREATE TABLE IF NOT EXISTS RegistrosRaciones ("
        "    id_registro INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    id_estudiante TEXT NOT NULL,"
        "    fecha_servicio TEXT NOT NULL,"        // "YYYY-MM-DD"
        "    tipo_racion INTEGER NOT NULL,"       // 0, 1, 2
        "    id_terminal TEXT NOT NULL,"
        "    hora_evento INTEGER NOT NULL,"         // epoch ms
        "    estado_registro INTEGER NOT NULL,"     // 0=PENDIENTE, 1=SINCRONIZADO
        "    FOREIGN KEY (id_estudiante) REFERENCES Usuarios (run_id),"
        "    UNIQUE (id_estudiante, fecha_servicio, tipo_racion)" // (RF2)
        ");"
        // Configuración global única 
        "CREATE TABLE IF NOT EXISTS ConfiguracionGlobal ("
        "    id_unica INTEGER PRIMARY KEY CHECK (id_unica = 1),"
        "    id_terminal TEXT NOT NULL,"
        "    tipo_racion INTEGER NOT NULL CHECK (tipo_racion IN (0,1,2))," // <--- CORREGIDO (Faltaba coma)
        "    puerto_impresora TEXT NOT NULL DEFAULT 'COM3'"                 // <--- CORREGIDO (Nombre Estandarizado)
        ");";

    if (!_ejecutar_sql_script(sql_schema)) {
        std::cerr << "[DB_Backend] ERROR: Falló la inicialización del esquema.\n";
        sqlite3_close(db_handle_);
        db_handle_ = nullptr;
        return false;
    }

    const char* sql_seed =
        "INSERT OR IGNORE INTO ConfiguracionGlobal "
        "(id_unica, id_terminal, tipo_racion, puerto_impresora) "
         "VALUES (1, 'NUC-01', 0, 'COM3');";
    
    if (!_ejecutar_sql_script(sql_seed)) {
        std::cerr << "[DB_Backend] WARN: Falló el 'seed' de configuración.\n";
    }

    std::cout << "[DB_Backend]: Base de datos inicializada en: " << db_path_ << "\n";
    return true;
}


//=== 2. API de Configuración (Tu diseño) ===

// (¡Tu código refactorizado, está perfecto!)
std::optional<Configuracion> DB_Backend::Obtener_Configuracion() {
    // El mutex se lockea aquí automáticamente
    std::lock_guard<std::mutex> lock(db_mutex_); 
    
    const char* sql =
        "SELECT id_terminal, tipo_racion, puerto_impresora FROM ConfiguracionGlobal WHERE id_unica = 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB_Backend] ERROR prepare Obtener_Configuracion: "
                  << sqlite3_errmsg(db_handle_) << "\n";
        return std::nullopt;
    }

    std::optional<Configuracion> out;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* t0 = sqlite3_column_text(stmt, 0);
        std::string id_terminal = t0 ? reinterpret_cast<const char*>(t0) : "";
        int v_tipo = sqlite3_column_int(stmt, 1);
        
        const unsigned char* t2= sqlite3_column_text(stmt, 2);
        std::string puerto_impresora = t2 ? reinterpret_cast<const char*>(t2) : "COM3";
        auto maybe = to_tipo_racion(v_tipo);
        
        if (maybe) {
            out = Configuracion{ id_terminal, *maybe, puerto_impresora };
        } else {
            std::cerr << "[DB_Backend] ERROR: tipo_racion inválido en BD: " << v_tipo << "\n";
            // Devolvemos el default en lugar de nada
            out = Configuracion{ id_terminal, TipoRacion::RACION_NO_APLICA, puerto_impresora };
        }
    } else if (rc != SQLITE_DONE) {
        std::cerr << "[DB_Backend] ERROR step Obtener_Configuracion: "
                  << sqlite3_errmsg(db_handle_) << "\n";
    }
    // Si SQLITE_DONE y no SQLITE_ROW, la tabla está vacía (no debería pasar)
    // 'out' seguirá siendo std::nullopt

    sqlite3_finalize(stmt);
    return out;
}

// (¡Tu código refactorizado, está perfecto!)
bool DB_Backend::Guardar_Configuracion(const Configuracion& config) {
    std::lock_guard<std::mutex> lock(db_mutex_);

    const char* sql =
        "UPDATE ConfiguracionGlobal "
        "SET id_terminal = ?1, tipo_racion = ?2, puerto_impresora = ?3 "
        "WHERE id_unica = 1;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB_Backend] ERROR prepare Guardar_Configuracion: "
                  << sqlite3_errmsg(db_handle_) << "\n";
        return false;
    }

    // Usar SQLITE_TRANSIENT le dice a SQLite que haga su propia copia del string.
    sqlite3_bind_text(stmt, 1, config.id_terminal.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, to_int(config.tipo_racion));
    sqlite3_bind_text(stmt, 3, config.puerto_impresora.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    bool exito = (rc == SQLITE_DONE);

    if (!exito) {
        std::cerr << "[DB_Backend] ERROR step Guardar_Configuracion: "
                  << sqlite3_errmsg(db_handle_) << "\n";
    }

    sqlite3_finalize(stmt);
    return exito; //exito y fin del programa
}

// --- 3. API de Hardware (El caché 1:N) ---

bool DB_Backend::Obtener_Todos_Templates(
    std::map<std::string, std::vector<uint8_t>>& /*out_templates*/
) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::cerr << "TODO: Implementar DB_Backend::Obtener_Todos_Templates\n";
    return false;
}


// --- 4. API de Lógica de Negocio (El Semáforo) ---

RolUsuario DB_Backend::Obtener_Perfil_Por_Run(
  const std::string& run_id,
  PerfilEstudiante& out_perfil
) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  const char* sql_join =
    "SELECT"
    "  U.dv, U.nombre_completo, U.id_rol, U.template_huella, "
    "  D.curso, D.es_pae "
    "FROM Usuarios U " // Usamos "Usuarios" (plural) como en tu CREATE TABLE
    // ¡Importante! Usamos LEFT JOIN para que puedas obtener Admins
    // (que no tienen entrada en DetallesEstudiante)
    "LEFT JOIN DetallesEstudiante D ON U.run_id = D.run_id " 
    "WHERE U.run_id = ?1;";

  sqlite3_stmt* stmt = nullptr;
  RolUsuario rol_encontrado = RolUsuario::Invalido; // 0 = No encontrado

  // Esta es la estructura try/catch/finalize correcta:
  try {
    // 1. Preparar la consulta
    if (sqlite3_prepare_v2(db_handle_, sql_join, -1, &stmt, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_handle_));
    }

    // 2. Asignar con BIND el run a factor ?1 del SQL
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_STATIC);

    // 3. Ejecutar la consulta
    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
      // Fila encontrada, mapear datos
      out_perfil.run_id = run_id;
      out_perfil.dv = sqlite3_column_text(stmt, 0)[0];
      out_perfil.nombre_completo = reinterpret_cast<const char*>(
        sqlite3_column_text(stmt, 1)
      );

      int rol_int = sqlite3_column_int(stmt, 2);
      rol_encontrado = to_rol_usuario(rol_int).value_or(RolUsuario::Invalido);
      out_perfil.id_rol = rol_encontrado;

      const void* blob_data = sqlite3_column_blob(stmt, 3);
      int blob_size = sqlite3_column_bytes(stmt, 3); // Se usa _bytes para el tamaño
      out_perfil.template_huella.assign(
        static_cast<const uint8_t*>(blob_data),
        static_cast<const uint8_t*>(blob_data) + blob_size
      );

      // Mapear detalles (pueden ser NULL si es LEFT JOIN)
      const unsigned char* curso_txt = sqlite3_column_text(stmt, 4);
      out_perfil.curso = curso_txt ? reinterpret_cast<const char*>(curso_txt) : "";
      out_perfil.es_pae = (sqlite3_column_int(stmt, 5) == 1);

    } else if (rc != SQLITE_DONE) {
      // Error en el step
      throw std::runtime_error(sqlite3_errmsg(db_handle_));
    }
    // Si es SQLITE_DONE (y no ROW), no se encontró = rol_encontrado es Invalido (correcto)

    } catch (const std::exception& e) {
        std::cerr << "[DB_Backend] ERROR al obtener perfil: " << e.what() << "\n";
        rol_encontrado = RolUsuario::Invalido; // Asegurar estado de error
    }

  // Finalize SIEMPRE debe correr, después del try/catch
    if (stmt) sqlite3_finalize(stmt);

    return rol_encontrado;
}

bool DB_Backend::Verificar_Racion_Doble(
    const std::string& run_id, 
    const std::string& fecha_iso,
    TipoRacion racion
) {
    // 1. Asegura la exclusividad del hilo
    std::lock_guard<std::mutex> lock(db_mutex_);

    // CRÍTICO: Definimos la variable SQL y corregimos el typo de la tabla
    const char* sql = 
        "SELECT EXISTS("
        "   SELECT 1 FROM RegistrosRaciones" // <-- Corregido: Tabla Plural
        "   WHERE id_estudiante = ?1"
        "     AND fecha_servicio = ?2"       // <-- Corregido: Placeholder correcto
        "     AND tipo_racion = ?3"
        ");";

    sqlite3_stmt* stmt = nullptr;
    bool is_duplicate = false; // Asume 'false' (no duplicado) por defecto
    
    // El try/catch debe envolver la lógica de la DB
    try {
        int rc;
        if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Preparar Verificar_Racion_Doble falló: " + 
                                     std::string(sqlite3_errmsg(db_handle_)));
        }

        // BIND: Parámetros de la consulta
        sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, fecha_iso.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, to_int(racion));

        rc = sqlite3_step(stmt);
        
        // Verifica si se encontró una fila (el resultado de EXISTS)
        if (rc == SQLITE_ROW) {
            is_duplicate = (sqlite3_column_int(stmt, 0) == 1); 
        } else if (rc != SQLITE_DONE) {
            throw std::runtime_error("Ejecutar Verificar_Racion_Doble falló: " + 
                                     std::string(sqlite3_errmsg(db_handle_)));
        }

    } catch (const std::exception& e) {
        // En caso de error, emitir WARN y devolver el default.
        std::cerr << "[DB_Backend] ERROR en Verificar_Racion_Doble: " << e.what() << "\n"; 
        is_duplicate = false;
    }

    // Finalize SIEMPRE debe correr
    if (stmt) {
        sqlite3_finalize(stmt);
    }

    // Retorna true si es duplicado, false si no.
    return is_duplicate;
}

bool DB_Backend::Guardar_Registro_Racion(const RegistroRacion& /*registro*/) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::cerr << "TODO: Implementar DB_Backend::Guardar_Registro_Racion\n";
    return false;
}


// --- 5. API de Sincronización (El Servicio de Sync) ---

std::vector<RegistroRacion> DB_Backend::Obtener_Registros_Pendientes() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::cerr << "TODO: Implementar DB_Backend::Obtener_Registros_Pendientes\n";
    return {}; // Retorna vector vacío
}

bool DB_Backend::Marcar_Registros_Sincronizados(
    const std::vector<std::int64_t>& /*ids_registros*/
) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::cerr << "TODO: Implementar DB_Backend::Marcar_Registros_Sincronizados\n";
    return false;
}


// --- 6. API de Administración (Enrolamiento) ---

bool DB_Backend::Enrolar_Estudiante_Completo(
    const RequestEnrolarUsuario& request
) {
    // 1. Asegura la exclusividad del hilo
    std::lock_guard<std::mutex> lock(db_mutex_);

    // Punteros para las sentencias preparadas
    sqlite3_stmt* stmt_user = nullptr;
    sqlite3_stmt* stmt_details = nullptr;
    bool success = false;

    // Inicia la transacción y envuelve todo en un bloque TRY/CATCH para el ROLLBACK
    try {
        if (!_ejecutar_sql_script("BEGIN TRANSACTION;")) { // <-- CORREGIDO: BEGIN
            throw std::runtime_error("No se pudo iniciar la transacción.");
        }

        // --- 1. INSERTAR EN TABLA USUARIOS ---
        // Se incluyen explícitamente las 6 columnas del struct Usuario
        const char* sql_user =
            "INSERT INTO Usuarios (run_id, dv, nombre_completo, id_rol, template_huella, password_hash) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6);"; // <-- CORREGIDO: SQL y comando INSERT

        if (sqlite3_prepare_v2(db_handle_, sql_user, -1, &stmt_user, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Preparar INSERT Usuarios falló: " + std::string(sqlite3_errmsg(db_handle_)));
        }

        // BIND: Estudiante (Rol=3), sin contraseña
        const char dv_array[2] ={request.dv_nuevo, '\0'};
        
        sqlite3_bind_text(stmt_user, 1, request.run_nuevo.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_user, 2, dv_array, 1, SQLITE_STATIC);        sqlite3_bind_text(stmt_user, 3, request.nombre_nuevo.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_user, 4, static_cast<int>(RolUsuario::Estudiante)); // Rol 3

        // BIND: Huella (BLOB)
        sqlite3_bind_blob(
            stmt_user, 
            5, 
            request.template_huella.data(), 
            static_cast<int>(request.template_huella.size()), 
            SQLITE_STATIC
        ); 
        
        // BIND: Password_hash (NULL para estudiante)
        sqlite3_bind_null(stmt_user, 6); // <-- CORREGIDO: Uso de sqlite3_bind_null

        if (sqlite3_step(stmt_user) != SQLITE_DONE) {
            throw std::runtime_error("Ejecutar INSERT Usuarios falló: " + std::string(sqlite3_errmsg(db_handle_)));
        }
        sqlite3_finalize(stmt_user);
        stmt_user = nullptr;

        const char* sql_details =
            "INSERT INTO DetallesEstudiante (run_id, curso, es_pae) "
            "VALUES (?1, ?2, ?3);";

        if (sqlite3_prepare_v2(db_handle_, sql_details, -1, &stmt_details, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Preparar INSERT Detalles falló: " + std::string(sqlite3_errmsg(db_handle_)));
        }

        // BIND: Detalles
        sqlite3_bind_text(stmt_details, 1, request.run_nuevo.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_details, 2, request.curso_nuevo.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_details, 3, request.es_pae ? 1 : 0);

        if (sqlite3_step(stmt_details) != SQLITE_DONE) {
            throw std::runtime_error("Ejecutar INSERT Detalles falló: " + std::string(sqlite3_errmsg(db_handle_)));
        }
        sqlite3_finalize(stmt_details);
        stmt_details = nullptr;

        // 3. FINALIZAR TRANSACCIÓN
        success = _ejecutar_sql_script("COMMIT;");
        if (!success) {
            throw std::runtime_error("COMMIT falló.");
        }

    } catch (const std::exception& e) {
        // En caso de CUALQUIER error, hacemos ROLLBACK y limpiamos.
        std::cerr << "[DB_Backend] ERROR fatal en Enrolamiento: " << e.what() << "\n";
        _ejecutar_sql_script("ROLLBACK;");
    }

    // Limpieza final de statements pendientes
    if (stmt_user) sqlite3_finalize(stmt_user);
    if (stmt_details) sqlite3_finalize(stmt_details);

    return success;
}

bool DB_Backend::Eliminar_Usuario(const std::string& /*run_id*/) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::cerr << "TODO: Implementar DB_Backend::Eliminar_Usuario\n";
    return false;
}


// ===== Helper Privado =====

// Esta es la función que estaba anidada. ,
bool DB_Backend::_ejecutar_sql_script(const char* sql) {
    // Nota: Esta función privada NO lockea el mutex,
    // porque es llamada por Inicializar_DB (que es single-thread)
    // o por otras funciones que YA lo lockearon.
    
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db_handle_, sql, nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB_Backend] ERROR sqlite3_exec: " 
                  << (zErrMsg ? zErrMsg : "(sin mensaje)") << "\n";
        if (zErrMsg) sqlite3_free(zErrMsg);
        return false;
    }
    return true; // Éxito
}