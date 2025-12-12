#pragma once

#include "DB_Backend.hpp"
#include "DB_models.hpp"
#include <vector>

struct EstadisticasGenerales {
  int count_desayunos;
  int count_almuerzos;
  int count_totales;
};

//estructura para el historial de un estudiante especifico
struct EstadisticasPersonales{
  std::string fecha;
  std::string hora;
  std::string tipo;
  std::string estado;
};

EstadisticasGenerales GetTodayStats(DB_Backend &db);
std::vector<PerfilEstudiante> GetAllStudents(DB_Backend &db);
std::vector<HistorialRacion> GetStudentHistory(DB_Backend &db, const std::string &run_id);
DB_Backend::EstadisticasEstudiante GetStudentStats(DB_Backend &db, const std::string &run_id);