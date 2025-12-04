#pragma once

#include "DB_Backend.hpp"
#include "Sensor.h"
#include <string>
#include <vector>

class HeadlessManager {
public:
  HeadlessManager(Sensor &sensor, DB_Backend &db);

  // Main loop that runs forever (until stop command)
  void run();

private:
  Sensor &sensor;
  DB_Backend &db;
  bool running;

  // --- Core Logic ---
  void checkSensorForTicket(); // The "Default Mode"
  void processInput();         // Check stdin for commands

  // --- Commands ---
  void handleCommand(const std::string &jsonCmd);
  void cmdEnroll(const std::string &payload);
  void cmdVerify();
  void cmdGetRecent();
  void cmdGetUnsynced();
  void cmdMarkSynced(const std::string &payload);

  // --- Helpers ---
  void sendJson(const std::string &key, const std::string &value);
  void sendJson(const std::string &key, int value);

  // Simple JSON parser helper (returns value for key)
  std::string getJsonValue(const std::string &json, const std::string &key);
};
