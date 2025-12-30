//CrowServer.cpp

#include "CrowServer.hpp"
#include "Endpoints.hpp"
#include "SensorWorker.h"
#include "TemplateManager.hpp"
#include "crow_all.h"
#include <iostream>
#include <vector>
#include <ctime>

void emitirTicket(const PerfilEstudiante& p, const std::string& fecha, const std::string& racion);
void imprimirUltimoTicket();

void runWebServer(DB_Backend &db) {
  crow::SimpleApp app;

  // =====================================================
  // INICIALIZAR SENSOR WORKER (HILO DEDICADO)
  // =====================================================
  std::cout << "[CrowServer] ----------------------------------------" << std::endl;
  std::cout << "[CrowServer] INICIANDO SENSOR WORKER..." << std::endl;
  
  // El worker maneja el sensor en un hilo separado
  static SensorWorker sensorWorker;
  
  bool sensorActivo = sensorWorker.start();
  
  if (sensorActivo) {
      std::cout << "[CrowServer] (+) SENSOR WORKER LISTO." << std::endl;
  } else {
      std::cerr << "[CrowServer] (-) ALERTA: Sensor no disponible." << std::endl;
      std::cerr << "[CrowServer]     El servidor continuará, pero sin huella." << std::endl;
  }
  std::cout << "[CrowServer] ----------------------------------------" << std::endl;

  // ===== STATIC FILES =====

  CROW_ROUTE(app, "/")
  ([]() {
    crow::response res;
    res.set_static_file_info("web/index.html");
    return res;
  });

  CROW_ROUTE(app, "/admin")
  ([]() {
    crow::response res;
    res.set_static_file_info("web/index.html");
    return res;
  });

  CROW_ROUTE(app, "/style.css")
  ([]() {
    crow::response res;
    res.set_static_file_info("web/style.css");
    res.set_header("Content-Type", "text/css");
    return res;
  });

  CROW_ROUTE(app, "/app.js")
  ([]() {
    crow::response res;
    res.set_static_file_info("web/app.js");
    res.set_header("Content-Type", "application/javascript");
    return res;
  });

  CROW_ROUTE(app, "/chart.main.js")
  ([]() {
    crow::response res;
    res.set_static_file_info("web/chart.main.js");
    res.set_header("Content-Type", "application/javascript");
    return res;
  });

  // ===== API ENDPOINTS =====

  CROW_ROUTE(app, "/api/dashboard")
  ([]() { return "Hola Bienvenido a crow"; });

  // Endpoint to get recent records with student full names
  CROW_ROUTE(app, "/api/recent")
  ([&db]() {
    auto registros = db.Obtener_Ultimos_Registros(10);
    crow::json::wvalue result;
    result["total"] = registros.size();

    std::vector<crow::json::wvalue> records;
    for (const auto &reg : registros) {
      crow::json::wvalue record;
      record["id"] = reg.id_registro;
      record["run"] = reg.id_estudiante;
      record["fecha"] = reg.fecha_servicio;
      record["tipo_racion"] = (int)reg.tipo_racion;
      record["terminal"] = reg.id_terminal;
      record["estado"] = (reg.estado_registro == EstadoRegistro::SINCRONIZADO)
                             ? "SINCRONIZADO"
                             : "PENDIENTE";

      // Get student profile to include full name
      PerfilEstudiante perfil;
      RolUsuario rol = db.Obtener_Perfil_Por_Run(reg.id_estudiante, perfil);
      if (rol != RolUsuario::Invalido) {
        record["nombre_completo"] = perfil.nombre_completo;
        record["curso"] = perfil.curso;
      } else {
        record["nombre_completo"] = "N/A";
        record["curso"] = "N/A";
      }

      records.push_back(std::move(record));
    }
    result["records"] = std::move(records);
    return result;
  });

  // Dashboard Stats
  CROW_ROUTE(app, "/api/stats")
  ([&db]() {
    auto stats = GetTodayStats(db);
    crow::json::wvalue result;
    result["desayunos"] = stats.count_desayunos;
    result["almuerzos"] = stats.count_almuerzos;
    result["total"] = stats.count_totales;
    return result;
  });

  CROW_ROUTE(app, "/api/Cmenu")
      .methods("POST"_method)([&db](const crow::request& req) {
          crow::json::wvalue result;
          
          auto x = crow::json::load(req.body);
          if (!x) {
              return crow::response(400, "JSON Invalido");
          }

          if (x.has("ping")) {
              result["status"] = "connected";
              result["message"] = "Cerebro C++ listo y escuchando";
              return crow::response(result);
          }

          if (x.has("option")) {
              int option = x["option"].i();
              
              if (option == 1) {
                  std::cout << "[C++] Iniciando secuencia de enrolamiento..." << std::endl;
                  result["status"] = "success";
                  result["action"] = "open_enrollment"; 
                  result["message"] = "Abriendo interfaz de huella...";
              }
              else {
                  result["status"] = "unknown_option";
              }
          }

          return crow::response(result);
      });

  // ==============================================
  // === VERIFICACION DE HUELLA - ENDPOINT ===
  // ==============================================
  CROW_ROUTE(app, "/api/verify_finger")
  ([&db]() {
    crow::json::wvalue result;
    const int UMBRAL_MATCH = 70;

    // Helper para crear respuesta JSON correctamente
    auto make_json_response = [](int code, crow::json::wvalue& json) {
        crow::response res(code, json.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    };

    // 1. Verificar que el sensor esté listo
    if (!sensorWorker.isReady()) {
        result["status"] = "error";
        result["message"] = "Sensor no disponible";
        return make_json_response(503, result);
    }

    // 2. Solicitar captura de huella (BLOQUEANTE pero thread-safe)
    CaptureResult captura = sensorWorker.captureBlocking(3000); // 3 segundos

    if (captura.status == VerifyResult::NO_FINGER) {
        // Timeout normal - no hay dedo
        result["type"] = "no_match";
        result["status"] = "waiting";
        return make_json_response(200, result);
    }
    
    if (captura.status != VerifyResult::SUCCESS) {
        // Error del sensor
        result["status"] = "error";
        result["message"] = captura.errorMessage;
        return make_json_response(500, result);
    }

    // 3. Tenemos huella - hacer identificación 1:N
    std::map<std::string, std::vector<uint8_t>> templates_bd;
    if (!db.Obtener_Todos_Templates(templates_bd)) {
        result["status"] = "error";
        result["message"] = "Error leyendo BD";
        return make_json_response(500, result);
    }

    std::string run_encontrado = "";

    for (const auto& [run, tmpl_bd] : templates_bd) {
        // Match usando el worker (thread-safe)
        int score = sensorWorker.matchBlocking(captura.templateData, tmpl_bd);
        
        if (score >= UMBRAL_MATCH) {
            run_encontrado = run;
            std::cout << "[verify_finger] Match encontrado: " << run 
                      << " (score: " << score << ")" << std::endl;
            break;
        }
    }

    if (run_encontrado.empty()) {
        result["type"] = "no_match";
        result["status"] = "rejected";
        return make_json_response(200, result);
    }

    // 4. LÓGICA DE NEGOCIO (Ticket)
    PerfilEstudiante perfil;
    db.Obtener_Perfil_Por_Run(run_encontrado, perfil);

    time_t now = time(0);
    tm *ltm = localtime(&now);
    TipoRacion tipo_racion = (ltm->tm_hour < 11) ? TipoRacion::Desayuno : TipoRacion::Almuerzo;
    std::string nombre_racion = (tipo_racion == TipoRacion::Desayuno) ? "Desayuno" : "Almuerzo";

    char buffer[20];
    strftime(buffer, 20, "%Y-%m-%d", ltm);
    std::string fecha_hoy(buffer);

    bool ya_comio = db.Verificar_Racion_Doble(run_encontrado, fecha_hoy, tipo_racion);

    if (ya_comio) {
        result["type"] = "ticket";
        result["status"] = "rejected_double";
        crow::json::wvalue data;
        data["nombre"] = perfil.nombre_completo;
        data["racion"] = nombre_racion;
        result["data"] = std::move(data);
    } else {
        RegistroRacion nuevoReg;
        nuevoReg.id_estudiante = run_encontrado;
        nuevoReg.fecha_servicio = fecha_hoy;
        nuevoReg.tipo_racion = tipo_racion;
        nuevoReg.id_terminal = "TOTEM-01";
        nuevoReg.hora_evento = (long long)now * 1000;
        nuevoReg.estado_registro = EstadoRegistro::PENDIENTE;

        if (db.Guardar_Registro_Racion(nuevoReg)) {
            std::cout << "[CrowServer]: Registro guardado. Generando Ticket Fisico" << std::endl;
            emitirTicket(perfil, fecha_hoy, nombre_racion);
            imprimirUltimoTicket();
            //--------------------------------
            result["type"] = "ticket";
            result["status"] = "approved";
            crow::json::wvalue data;
            data["nombre"] = perfil.nombre_completo;
            data["curso"] = perfil.curso;
            data["run"] = perfil.run_id;
            data["racion"] = nombre_racion;
            result["data"] = std::move(data);
        } else {
            result["status"] = "error";
            result["message"] = "Error guardando en BD";
        }
    }

    std::cout << "[verify_finger] Enviando respuesta: " << result.dump() << std::endl;
    return make_json_response(200, result);
  });

  // ==============================================
  // === ESTADO DEL SENSOR ===
  // ==============================================
  CROW_ROUTE(app, "/api/sensor/status")
  ([]() {
    crow::json::wvalue result;
    
    bool ready = sensorWorker.isReady();
    bool running = sensorWorker.isRunning();
    
    result["available"] = ready;
    result["worker_running"] = running;
    result["message"] = ready ? "Sensor conectado y listo" : 
                        (running ? "Worker activo pero sensor no disponible" : 
                                   "Worker no iniciado");
    
    crow::response res(200, result.dump());
    res.set_header("Content-Type", "application/json");
    return res;
  });

  // ===== STUDENT MANAGEMENT =====

  // GET all students
  CROW_ROUTE(app, "/api/students").methods("GET"_method)([&db]() {
    auto students = GetAllStudents(db);
    std::vector<crow::json::wvalue> list;
    for (const auto &s : students) {
      crow::json::wvalue student;
      student["run"] = s.run_id;
      student["nombre"] = s.nombre_completo;
      student["curso"] = s.curso;
      student["activo"] = true;
      list.push_back(std::move(student));
    }
    crow::json::wvalue result;
    result = std::move(list);
    return result;
  });

  // GET student history by run_id
  CROW_ROUTE(app, "/api/students/<string>/history")
  .methods("GET"_method)([&db](const std::string &run_id) {
    auto historial = GetStudentHistory(db, run_id);
    std::vector<crow::json::wvalue> records;
    
    for (const auto &reg : historial) {
      crow::json::wvalue record;
      record["fecha"] = reg.fecha;
      record["hora"] = reg.hora;
      record["tipo"] = reg.tipo;
      record["estado"] = reg.estado;
      records.push_back(std::move(record));
    }
    
    crow::json::wvalue result = crow::json::wvalue::list();
    result = std::move(records);
    return crow::response(200, result);
  });

  // POST - Enroll new student WITH FINGERPRINT CAPTURE
  CROW_ROUTE(app, "/api/students")
      .methods("POST"_method)([&db](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        std::string run = body["run"].s();
        std::string nombre = body["nombre"].s();
        std::string curso = body["curso"].s();

        std::cout << "[CrowServer] Iniciando enrolamiento de: " << nombre
                  << " (RUN: " << run << ")" << std::endl;

        // Verificar sensor
        if (!sensorWorker.isReady()) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Sensor de huellas no disponible";
            error["error_code"] = "SENSOR_NOT_READY";
            return crow::response(503, error);
        }

        // Capturar huella usando el worker
        std::cout << "[CrowServer] Esperando huella..." << std::endl;
        CaptureResult captura = sensorWorker.captureBlocking(15000); // 15 segundos para enrolamiento

        if (captura.status != VerifyResult::SUCCESS) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = (captura.status == VerifyResult::NO_FINGER) 
                ? "No se detectó huella. Intente de nuevo."
                : "Error del sensor: " + captura.errorMessage;
            error["error_code"] = "FINGERPRINT_CAPTURE_FAILED";
            return crow::response(408, error);
        }

        std::cout << "[CrowServer] Huella capturada. Tamaño: " 
                  << captura.templateData.size() << " bytes" << std::endl;

        // Extraer dígito verificador del RUN
        char dv = 'K';
        if (!run.empty()) {
          char last_char = run.back();
          if (std::isalpha(last_char) || std::isdigit(last_char)) {
            dv = std::toupper(last_char);
          }
        }

        // Crear request con la huella capturada
        RequestEnrolarUsuario request;
        request.run_nuevo = run;
        request.dv_nuevo = dv;
        request.nombre_nuevo = nombre;
        request.curso_nuevo = curso;
        request.template_huella = captura.templateData;

        // Guardar en la base de datos
        bool success = db.Enrolar_Estudiante_Completo(request);

        if (success) {
          std::cout << "[CrowServer] ✓ Estudiante enrolado exitosamente" << std::endl;
          crow::json::wvalue result;
          result["success"] = true;
          result["message"] = "Estudiante enrolado exitosamente";
          result["fingerprint_size"] = (int)captura.templateData.size();
          return crow::response(200, result);
        } else {
          crow::json::wvalue error;
          error["success"] = false;
          error["message"] = "Error al guardar (RUN duplicado o error de BD)";
          error["error_code"] = "DATABASE_ERROR";
          return crow::response(500, error);
        }
      });

  // PUT - Update student
  CROW_ROUTE(app, "/api/students/<string>")
      .methods("PUT"_method)([&db](const crow::request &req, std::string run) {
        auto body = crow::json::load(req.body);
        if (!body) {
          return crow::response(400, "Invalid JSON");
        }

        std::string nuevo_nombre = body["nombre"].s();
        bool success = db.Actualizar_Estudiante(run, nuevo_nombre);

        if (success) {
          crow::json::wvalue result;
          result["success"] = true;
          result["message"] = "Estudiante actualizado exitosamente";
          return crow::response(200, result);
        } else {
          crow::json::wvalue error;
          error["success"] = false;
          error["message"] = "Error al actualizar estudiante";
          return crow::response(500, error);
        }
      });

  // DELETE - Delete student
  CROW_ROUTE(app, "/api/students/<string>")
      .methods("DELETE"_method)([&db](std::string run) {
        bool success = db.Eliminar_Usuario(run);

        if (success) {
          crow::json::wvalue result;
          result["success"] = true;
          result["message"] = "Estudiante eliminado exitosamente";
          return crow::response(200, result);
        } else {
          crow::json::wvalue error;
          error["success"] = false;
          error["message"] = "Error al eliminar estudiante";
          return crow::response(500, error);
        }
      });

  // GET - Student statistics
  CROW_ROUTE(app, "/api/students/<string>/stats")
  ([&db](std::string run) {
    auto stats = db.Obtener_Estadisticas_Estudiante(run);

    crow::json::wvalue result;
    result["desayunos"] = stats.count_desayunos;
    result["almuerzos"] = stats.count_almuerzos;
    result["total"] = stats.count_total;
    result["porcentaje_asistencia"] = stats.porcentaje_asistencia;

    return result;
  });

  // ===== EXPORT =====

  // Export students to CSV
  CROW_ROUTE(app, "/api/export/students")
  ([&db]() {
    auto students = GetAllStudents(db);

    std::string csv = "RUN,Nombre,Curso\n";
    for (const auto &s : students) {
      csv += s.run_id + "," + s.nombre_completo + "," + s.curso + "\n";
    }

    crow::response res(csv);
    res.add_header("Content-Type", "text/csv");
    res.add_header("Content-Disposition",
                   "attachment; filename=estudiantes.csv");
    return res;
  });

  // Export records to CSV
  CROW_ROUTE(app, "/api/export/records")
  ([&db]() {
    auto records = db.Obtener_Ultimos_Registros(1000);

    std::string csv = "ID,RUN,Fecha,Tipo Racion,Terminal,Estado\n";
    for (const auto &r : records) {
      std::string tipo =
          (r.tipo_racion == TipoRacion::Desayuno) ? "Desayuno" : "Almuerzo";
      std::string estado = (r.estado_registro == EstadoRegistro::SINCRONIZADO)
                               ? "SINCRONIZADO"
                               : "PENDIENTE";

      csv += std::to_string(r.id_registro) + "," + r.id_estudiante + "," +
             r.fecha_servicio + "," + tipo + "," + r.id_terminal + "," +
             estado + "\n";
    }

    crow::response res(csv);
    res.add_header("Content-Type", "text/csv");
    res.add_header("Content-Disposition", "attachment; filename=registros.csv");
    return res;
  });

  // ===== START SERVER =====

  std::cout << "[CrowServer] Iniciando en puerto 18080..." << std::endl;
  std::cout << "[CrowServer] Presiona Ctrl+C para detener." << std::endl;
  
  app.port(18080).multithreaded().run();
  
  // Cuando el servidor se detenga, detener el worker
  sensorWorker.stop();
}