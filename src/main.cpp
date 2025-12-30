//C:\Digitador\src\main.cpp

#include "CrowServer.hpp"
#include "DB_Backend.hpp"
#include "HeadlessManager.hpp"
#include "Sensor.h"
#include "TemplateManager.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Function to print the main menu
void printMenu() {
  std::cout << "\n========= MENU HUELLERO DIGITADOR PIAMARTA =========\n";
  std::cout << "1) Enrolar alumno\n";
  std::cout << "2) Verificar huella\n";
  std::cout << "3) Procesar ticket\n";
  std::cout << "4) Iniciar Servidor Web (Test)\n";
  std::cout << "5) Ver ultimos registros\n";
  std::cout << "6) Borrar Ultimos Registros\n";
  std::cout << "7) Salir\n";
  std::cout << "\n========= OPCION DE RIESGO =======================\n";
  std::cout << "10) Borrar todos los datos";
  std::cout << "\n==================================================\n";
  std::cout << "Seleccione una opción: ";
}

int main(int argc, char *argv[]) {
  // Check for webserver mode
  bool webserverMode = false;
  if (argc > 1 && strcmp(argv[1], "--webserver") == 0) {
    webserverMode = true;
  }

  // Ruta de la base de datos (ajusta si usas otra)
  DB_Backend db("BD/digitador.db");
  if (!db.Inicializar_DB()) {
    std::cerr << "(-) No se pudo inicializar la base de datos.\n";
    return 1;
  }

  // If webserver mode, skip sensor initialization and go straight to server
  if (webserverMode) {
    runWebServer(db);
    return 0;
  }

  Sensor sensor;
  if (!sensor.initSensor()) {
    std::cerr << "(-) No se pudo inicializar el sensor de huella.\n";
    return 1;
  }

  // Check for headless mode (for Totem)
  if (argc > 1 && strcmp(argv[1], "--headless") == 0) {
    HeadlessManager headless(sensor, db);
    headless.run();
    return 0;
  }

  system("CLS");

  bool running = true;
  while (running) {
    printMenu();
    int opt = 0;
    std::cin >> opt;
    system("CLS");

    switch (opt) {
    case 1: //enrolar alumno
      menuEnroll(sensor, db);
      system("CLS");
      break;
    case 2: //verificar alumno
      menuVerify(sensor, db);
      system("CLS");
      break;
    case 3: //procesar ticket
      menuProcessTicket(sensor, db);
      system("CLS");
      break;
    case 4: //salir del sistema
      runWebServer(db);
      system("CLS");
      break;
    case 5: //mostrar todos los registros recientes
      menuShowRecent(db);
      system("CLS");
      break;
    case 6: // Eliminar los ultimos registros
      if (db.borrarRegistros()) {
          std::cout << "Registros de raciones eliminados correctamente.\n";
      } else {
          std::cout << "Hubo un error al intentar borrar los registros.\n";
      }      
      system("PAUSE");
      system("CLS");
      break;

    case 7: //salir del sistema
      running = false;
      system("CLS");
      break;
      
    case 10: // Borrar todos los datos
    {
      char r1;
      std::cout << "\n ADVERTENCIA: ¿esta seguro de borrar TODOS los datos? (s/n): ";
      std::cin >> r1;
    
      if (tolower(r1) == 's') {
        std::cout << "¿SEGURO SEGURO? Escribe S otra vez: ";
        char r2;
        std::cin >> r2;
        
        if (tolower(r2) == 's') {
            std::cout << "Borrando...\n";
            db.borrarTodo();
            std::cout << "Hecho.\n";
      } else {
            std::cout << "Cancelado.\n";
        }
      } else {
        std::cout << "Cancelado.\n";
      }
    
      std::cout << "\nEnter para continuar...";
      std::cin.ignore();
      std::cin.get();
      system("CLS");
      break;
    }

    default:
      std::cout << "Opción inválida. Intente nuevamente.\n";
      system("CLS");
      break;
    }
  }

  sensor.closeSensor();
  std::cout << "Programa terminado.\n";
  return 0;
}
