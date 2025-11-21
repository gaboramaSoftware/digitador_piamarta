//simulador de proceso de huellero

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unistd.h> //para sleep
#include "db_models.hpp" //para las estructuras de datos

//simular hardware 

static void* G_ZKTeco_Handle = nullptr;
static bool G_Sensor_Initialized = false;