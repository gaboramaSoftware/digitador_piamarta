#pragma once

#include "DB_Backend.hpp"
#include <vector>

struct EstadisticasGenerales {
  int count_desayunos;
  int count_almuerzos;
  int count_totales;
};

EstadisticasGenerales GetTodayStats(DB_Backend &db);
std::vector<PerfilEstudiante> GetAllStudents(DB_Backend &db);