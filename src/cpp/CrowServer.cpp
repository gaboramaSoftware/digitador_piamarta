#include "CrowServer.hpp"
#include "Endpoints.hpp"
#include "crow_all.h"
#include "sensor.h" // Para captura de huellas
#include <iostream>
#include <vector>

void runWebServer(DB_Backend &db) {
  crow::SimpleApp app;

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

        // --- CAPTURA DE HUELLA CON HARDWARE ---
        std::vector<unsigned char> huella_capturada;

        try {
          Sensor sensor;

          // Inicializar el sensor
          if (!sensor.initSensor()) {
            std::cerr << "[CrowServer] ERROR: Sensor no disponible"
                      << std::endl;
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Error: Sensor de huellas no disponible. "
                               "Verifique la conexión USB.";
            error["error_code"] = "SENSOR_INIT_FAILED";
            return crow::response(503, error);
          }

          std::cout << "[CrowServer] Sensor inicializado. Esperando huella..."
                    << std::endl;

          // Configurar timeout
          sensor.setTimeout(15000);    // 15 segundos
          sensor.setPollInterval(200); // Chequear cada 200ms

          // CAPTURAR HUELLA (función bloqueante)
          bool captura_exitosa =
              sensor.capturenCreateTemplate(huella_capturada);

          // Cerrar el sensor
          sensor.closeSensor();

          if (!captura_exitosa || huella_capturada.empty()) {
            std::cerr << "[CrowServer] ERROR: No se pudo capturar la huella"
                      << std::endl;
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] =
                "No se detectó huella en el sensor. Por favor, coloque su dedo "
                "correctamente e intente de nuevo.";
            error["error_code"] = "FINGERPRINT_TIMEOUT";
            return crow::response(408, error);
          }

          std::cout << "[CrowServer] Huella capturada exitosamente. Tamaño: "
                    << huella_capturada.size() << " bytes" << std::endl;

        } catch (const std::exception &e) {
          std::cerr << "[CrowServer] EXCEPCIÓN al capturar huella: " << e.what()
                    << std::endl;
          crow::json::wvalue error;
          error["success"] = false;
          error["message"] = "Error interno al procesar la huella";
          error["error_code"] = "SENSOR_EXCEPTION";
          return crow::response(500, error);
        }

        // --- GUARDAR EN BASE DE DATOS ---

        // Extraer dígito verificador del RUN
        char dv = 'K';
        if (!run.empty()) {
          char last_char = run.back();
          if (std::isalpha(last_char) || std::isdigit(last_char)) {
            dv = std::toupper(last_char);
          }
        }

        // Crear request con la huella REAL capturada
        RequestEnrolarUsuario request;
        request.run_nuevo = run;
        request.dv_nuevo = dv;
        request.nombre_nuevo = nombre;
        request.curso_nuevo = curso;
        request.template_huella = huella_capturada;

        // Guardar en la base de datos
        bool success = db.Enrolar_Estudiante_Completo(request);

        if (success) {
          std::cout << "[CrowServer] ✓ Estudiante enrolado exitosamente"
                    << std::endl;
          crow::json::wvalue result;
          result["success"] = true;
          result["message"] =
              "Estudiante enrolado exitosamente con huella dactilar";
          result["fingerprint_size"] = (int)huella_capturada.size();
          return crow::response(200, result);
        } else {
          std::cerr << "[CrowServer] ERROR: Fallo al guardar en BD"
                    << std::endl;
          crow::json::wvalue error;
          error["success"] = false;
          error["message"] =
              "Error al guardar el estudiante (RUN duplicado o error de BD)";
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

  // ===== SENSOR STATUS =====

  CROW_ROUTE(app, "/api/sensor/status")
  ([]() {
    crow::json::wvalue result;

    try {
      Sensor sensor;
      bool sensor_ok = sensor.initSensor();
      sensor.closeSensor();

      result["available"] = sensor_ok;
      result["message"] =
          sensor_ok ? "Sensor conectado y listo" : "Sensor no disponible";

      return crow::response(200, result);
    } catch (const std::exception &e) {
      result["available"] = false;
      result["message"] = "Error al verificar sensor";
      result["error"] = e.what();
      return crow::response(500, result);
    }
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

  std::cout
      << "Abriendo servidor en puerto: 18080... (Presiona Ctrl+C para parar)\n";
  app.port(18080).multithreaded().run();
}
