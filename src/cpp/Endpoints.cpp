//endpoints.cpp

#include "Endpoints.hpp"
#include "DB_models.hpp"
#include <iomanip>
#include <sstream>

EstadisticasGenerales GetTodayStats(DB_Backend &db) {
  auto stats = db.Obtener_Estadisticas_Hoy();
  return {stats.count_desayunos, stats.count_almuerzos, stats.count_total};
}

std::vector<PerfilEstudiante> GetAllStudents(DB_Backend &db) {
  return db.Obtener_Todos_Estudiantes();
}

// Obtener historial completo de 1 estudiante
std::vector<HistorialRacion> GetStudentHistory(DB_Backend &db, const std::string &run_id) {
  return db.Obtener_Historial_Estudiante(run_id);
}

//Wrapper para estadísticas de 1 solo estudiante
DB_Backend::EstadisticasEstudiante GetStudentStats(DB_Backend &db, const std::string &run_id) {
  return db.Obtener_Estadisticas_Estudiante(run_id);
}