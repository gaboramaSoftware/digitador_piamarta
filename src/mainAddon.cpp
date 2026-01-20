#include <napi.h>
#include <iostream>
#include <cstdio>
#include "./DB_Backend.hpp"
#include "./Sensor.h"

//punteros globales
DB_Backend* globalDB = nullptr;
Sensor* globalSensor = nullptr;

//inicializar funciones
Napi: 