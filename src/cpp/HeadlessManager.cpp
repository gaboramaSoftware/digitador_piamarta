//HeadlessManager.cpp

#include "HeadlessManager.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <windows.h> // For PeekNamedPipe

// Helper to check if stdin has data without blocking
bool isStdinReady() {
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  if (hStdin == INVALID_HANDLE_VALUE)
    return false;

  DWORD bytesAvailable = 0;
  if (!PeekNamedPipe(hStdin, NULL, 0, NULL, &bytesAvailable, NULL)) {
    return false;
  }
  return bytesAvailable > 0;
}

HeadlessManager::HeadlessManager(Sensor &sensor, DB_Backend &db)
    : sensor(sensor), db(db), running(true) {}

void HeadlessManager::run() {
  // Notify start
  sendJson("status", "ready");

  while (running) {
    processInput();
    checkSensorForTicket();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void HeadlessManager::processInput() {
  if (isStdinReady()) {
    std::string line;
    if (std::getline(std::cin, line)) {
      if (!line.empty()) {
        handleCommand(line);
      }
    }
  }
}

void HeadlessManager::checkSensorForTicket() {
  // 1. Try to capture finger
  std::vector<unsigned char> tpl;
  if (!sensor.acquireFingerprintImmediate(tpl)) {
    return; // No finger or error
  }

  // 2. Finger detected! Identify.
  sendJson("status", "processing_finger");

  // Load all templates (Optimization: Cache this in memory instead of loading
  // every time)
  std::map<std::string, std::vector<uint8_t>> templatesMap;
  if (!db.Obtener_Todos_Templates(templatesMap)) {
    sendJson("error", "db_error");
    return;
  }

  std::string bestRun;
  int bestScore = -1;

  for (auto &[run, storedTpl] : templatesMap) {
    std::vector<unsigned char> storedUchar(storedTpl.begin(), storedTpl.end());
    int score = sensor.matchTemplate(tpl, storedUchar);
    if (score > bestScore) {
      bestScore = score;
      bestRun = run;
    }
  }

  if (bestScore >= 60) { // Threshold
    // Match found!
    PerfilEstudiante perfil;
    db.Obtener_Perfil_Por_Run(bestRun, perfil);

    // --- BUSINESS LOGIC START ---

    // 1. Determine Ration Type based on Time
    auto now_tp = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now_tp);
    tm *ltm = localtime(&now_time_t);
    int hour = ltm->tm_hour;

    TipoRacion tipoRacion = TipoRacion::RACION_NO_APLICA;
    std::string rationName = "Desconocido";

    if (hour >= 8 && hour < 12) {
      tipoRacion = TipoRacion::Desayuno;
      rationName = "Desayuno";
    } else if (hour >= 12) {
      tipoRacion = TipoRacion::Almuerzo;
      rationName = "Almuerzo";
    } else {
      // Outside of service hours
      std::cout << "{\"type\":\"ticket\", \"status\":\"rejected_time\", "
                   "\"data\":{\"run\":\""
                << perfil.run_id << "\", \"nombre\":\""
                << perfil.nombre_completo << "\"}}" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      return;
    }

    std::cout << "[DEBUG] Current hour: " << ltm->tm_hour
              << " | racion: " << (int)tipoRacion << "| Name: " << rationName
              << std::endl;

    // 2. Get Date ISO for DB Check
    char dateBuffer[20];
    strftime(dateBuffer, 20, "%Y-%m-%d", ltm);
    std::string fechaHoy(dateBuffer);

    // 3. Check Double Ration
    if (db.Verificar_Racion_Doble(perfil.run_id, fechaHoy, tipoRacion)) {
      // Already ate this ration today
      std::cout << "{\"type\":\"ticket\", \"status\":\"rejected_double\", "
                   "\"data\":{\"run\":\""
                << perfil.run_id << "\", \"nombre\":\""
                << perfil.nombre_completo << "\", \"racion\":\"" << rationName
                << "\"}}" << std::endl;
    } else {
      // 4. Save Ticket
      auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now_tp.time_since_epoch())
                        .count();

      RegistroRacion nuevoTicket;
      nuevoTicket.id_estudiante = perfil.run_id;
      nuevoTicket.fecha_servicio = fechaHoy;
      nuevoTicket.tipo_racion = tipoRacion;
      nuevoTicket.id_terminal = "TOTEM-01"; // TODO: Get from Config
      nuevoTicket.hora_evento = now_ms;
      nuevoTicket.estado_registro = EstadoRegistro::PENDIENTE;

      if (db.Guardar_Registro_Racion(nuevoTicket)) {
        // Success!
        std::cout << "{\"type\":\"ticket\", \"status\":\"approved\", "
                     "\"data\":{\"run\":\""
                  << perfil.run_id << "\", \"nombre\":\""
                  << perfil.nombre_completo << "\", \"curso\":\""
                  << perfil.curso << "\", \"racion\":\"" << rationName << "\"}}"
                  << std::endl;
      } else {
        sendJson("error", "db_save_failed");
      }
    }
    // --- BUSINESS LOGIC END ---

  } else {
    sendJson("type", "no_match");
  }

  // Sleep a bit to avoid multiple reads of same finger
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

void HeadlessManager::handleCommand(const std::string &jsonCmd) {
  std::string cmd = getJsonValue(jsonCmd, "cmd");

  if (cmd == "stop") {
    running = false;
  } else if (cmd == "enroll") {
    cmdEnroll(jsonCmd);
  } else if (cmd == "verify") {
    sendJson("msg", "verifying_mode");
  } else if (cmd == "get_recent") {
    cmdGetRecent();
  } else if (cmd == "get_unsynced") {
    cmdGetUnsynced();
  } else if (cmd == "mark_synced") {
    cmdMarkSynced(jsonCmd);
  } else {
    sendJson("error", "unknown_cmd");
  }
}

void HeadlessManager::cmdEnroll(const std::string &payload) {
  std::string run = getJsonValue(payload, "run");
  if (run.empty()) {
    sendJson("error", "missing_run");
    return;
  }

  std::string nombre = getJsonValue(payload, "nombre");
  std::string curso = "1A"; // Default for now

  sendJson("msg", "place_finger_enroll");

  std::vector<unsigned char> tpl;
  if (sensor.capturenCreateTemplate(tpl)) {
    RequestEnrolarUsuario req;
    req.run_nuevo = run;
    req.dv_nuevo = 'K'; // TODO: Parse DV
    req.nombre_nuevo = nombre;
    req.curso_nuevo = curso;
    req.template_huella = tpl;

    if (db.Enrolar_Estudiante_Completo(req)) {
      sendJson("status", "enroll_success");
    } else {
      sendJson("error", "db_save_failed");
    }
  } else {
    sendJson("error", "enroll_timeout");
  }
}

void HeadlessManager::cmdGetRecent() {
  auto recents = db.Obtener_Ultimos_Registros(100);
  std::cout << "{\"type\":\"recent_data\", \"data\":[";
  for (size_t i = 0; i < recents.size(); ++i) {
    const auto &r = recents[i];
    std::cout << "{\"id\":" << r.id_registro << ",\"run\":\"" << r.id_estudiante
              << "\",\"fecha\":\"" << r.fecha_servicio
              << "\",\"tipo\":" << (int)r.tipo_racion
              << ",\"hora\":" << r.hora_evento << "}";
    if (i < recents.size() - 1)
      std::cout << ",";
  }
  std::cout << "]}" << std::endl;
}

void HeadlessManager::cmdGetUnsynced() {
  auto pendientes = db.Obtener_Registros_Pendientes();
  std::cout << "{\"type\":\"sync_data\", \"data\":[";
  for (size_t i = 0; i < pendientes.size(); ++i) {
    const auto &r = pendientes[i];
    std::cout << "{\"id\":" << r.id_registro << ",\"run\":\"" << r.id_estudiante
              << "\",\"fecha\":\"" << r.fecha_servicio
              << "\",\"tipo\":" << (int)r.tipo_racion << "}";
    if (i < pendientes.size() - 1)
      std::cout << ",";
  }
  std::cout << "]}" << std::endl;
}

void HeadlessManager::cmdMarkSynced(const std::string &payload) {
  std::vector<int64_t> ids;
  size_t pos = payload.find("[");
  if (pos != std::string::npos) {
    size_t end = payload.find("]", pos);
    if (end != std::string::npos) {
      std::string content = payload.substr(pos + 1, end - pos - 1);
      std::string numStr;
      for (char c : content) {
        if (isdigit(c)) {
          numStr += c;
        } else if (c == ',' && !numStr.empty()) {
          ids.push_back(std::stoll(numStr));
          numStr.clear();
        }
      }
      if (!numStr.empty())
        ids.push_back(std::stoll(numStr));
    }
  }

  if (db.Marcar_Registros_Sincronizados(ids)) {
    sendJson("status", "sync_marked_ok");
  } else {
    sendJson("error", "sync_mark_failed");
  }
}

void HeadlessManager::sendJson(const std::string &key,
                               const std::string &value) {
  std::cout << "{\"" << key << "\":\"" << value << "\"}" << std::endl;
}

void HeadlessManager::sendJson(const std::string &key, int value) {
  std::cout << "{\"" << key << "\":" << value << "}" << std::endl;
}

std::string HeadlessManager::getJsonValue(const std::string &json,
                                          const std::string &key) {
  std::string search = "\"" + key + "\":\"";
  size_t start = json.find(search);
  if (start == std::string::npos)
    return "";

  start += search.length();
  size_t end = json.find("\"", start);
  if (end == std::string::npos)
    return "";

  return json.substr(start, end - start);
}
