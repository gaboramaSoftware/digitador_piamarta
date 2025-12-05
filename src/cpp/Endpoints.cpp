#include "Endpoints.hpp"

EstadisticasGenerales GetTodayStats(DB_Backend &db) {
  auto stats = db.Obtener_Estadisticas_Hoy();
  return {stats.count_desayunos, stats.count_almuerzos, stats.count_total};
}

std::vector<PerfilEstudiante> GetAllStudents(DB_Backend &db) {
  return db.Obtener_Todos_Estudiantes();
}
