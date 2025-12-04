#ifndef CROW_SERVER_HPP
#define CROW_SERVER_HPP

#include "DB_Backend.hpp"

// Inicia el servidor web Crow en el puerto 18080
// Expone endpoints REST para consultar datos de la base de datos
void runWebServer(DB_Backend &db);

#endif // CROW_SERVER_HPP
