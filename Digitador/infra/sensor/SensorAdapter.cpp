// adaptamos el sensor para recibir un buffer de datos especifico

#include "../../core/Hardware/Sensor/Sensor.h"
#include <algorithm>
#include <vector>

extern "C" {
Sensor *sensor_new() { return new Sensor(); }
bool sensor_init(Sensor *s) { return s->initSensor(); }

// Función para capturar y devolver el tamaño del template
int sensor_capture(Sensor *s, unsigned char *buffer) {
  std::vector<unsigned char> data;
  if (s->captureCreateTemplate(data)) {
    std::copy(data.begin(), data.end(), buffer);
    return data.size();
  }
  return -1;
}
}