#include "CrowServer.hpp"
#include "crow_all.h"
#include <iostream>
#include <vector>

void runWebServer(DB_Backend &db) {
  crow::SimpleApp app;

  // Serve static files for admin panel
  CROW_ROUTE(app, "/")
  ([]() {
    crow::response res;
    res.set_static_file_info("src/web/index.html");
    return res;
  });

  CROW_ROUTE(app, "/admin")
  ([]() {
    crow::response res;
    res.set_static_file_info("src/web/index.html");
    return res;
  });

  CROW_ROUTE(app, "/style.css")
  ([]() {
    crow::response res;
    res.set_static_file_info("src/web/style.css");
    return res;
  });

  CROW_ROUTE(app, "/app.js")
  ([]() {
    crow::response res;
    res.set_static_file_info("src/web/app.js");
    return res;
  });

  // dashboard endpoint (API test)
  CROW_ROUTE(app, "/api/dashboard")([]() { return "Hola Bienvenido a crow"; });

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

  std::cout
      << "Abriendo servidor en puerto: 18080... (Presiona Ctrl+C para parar)\n";
  app.port(18080).multithreaded().run();
}
