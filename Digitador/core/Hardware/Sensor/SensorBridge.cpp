// internal/adapters/sensor/sensor_bridge.cpp
// archivo para crear el puente entre C++ y Go
#include "SensorBridge.h"
#include "Sensor.h" // Tu archivo original

extern "C" {

// creamos un objeto sensor
SensorHandle CreateSensor() {
  return new Sensor(); // Creamos la instancia de C++
}

// creamos el destructor del sensor
void DestroySensor(SensorHandle handle) {
  // si existia un sensor previamente?
  if (handle) {
    // si, entonces lo borramos el sensor antiguo
    delete static_cast<Sensor *>(handle);
  }
}

// inicializamos el sensor creado
int InitSensor(SensorHandle handle) {
  // puede iniciar el sensor?
  if (!handle)
    // no, entonces retornamos 0
    return 0; // 0 = false
  // si, entonces convertimos el puntero a clase sensor
  Sensor *s = static_cast<Sensor *>(handle);
  // y retornamos el resultado de esta conversion
  return s->initSensor() ? 1 : 0;
}

//====AHORA TRABAJAMOS CON CGO====//

// capturamos la huella del sensor, ahaora con el metodo inmediato
int AcquireFingerprint(SensorHandle handle, unsigned char *outBuffer,
                       int *outSize) {
  // el sensor existe?
  if (!handle)
    return 0;
  // si existe, convertimos el puntero a clase sensor
  Sensor *s = static_cast<Sensor *>(handle);

  // creamos un vector para almacenar el template
  std::vector<unsigned char> templateData;
  // Usamos el método inmediato, Go se encargará de hacer el bucle (polling)
  if (s->captureTemplateImmediate(templateData)) {
    // guardamos el tamaño del template en un puntero
    *outSize = templateData.size();
    // Movemos los bytes al buffer de Go
    std::copy(templateData.begin(), templateData.end(), outBuffer);
    return 1; // Éxito
  }
  return 0; // Falla (no hay dedo)
}

// comparamos dos huellas
int MatchTemplates(SensorHandle handle, const unsigned char *tpl1, int size1,
                   const unsigned char *tpl2, int size2) {

  // el sensor existe?
  if (!handle)
    return -1;

  //===AQUI TRABAJAMOS CON GO===

  // si existe, convertimos el puntero a clase sensor
  Sensor *s = static_cast<Sensor *>(handle);

  // Reconstruimos los vectores temporales para que tu código original funcione
  std::vector<unsigned char> v1(tpl1, tpl1 + size1);
  std::vector<unsigned char> v2(tpl2, tpl2 + size2);
  // comparamos las 2 huellas sin el return
  int score = s->matchTemplate(v1, v2);
  // devolvemos el score
  return score;
}

// 1:N - Agregar template al cache interno
int DBAdd(SensorHandle handle, int userId, unsigned char *fpTemplate,
          int cbTemplate) {
  if (!handle)
    return 0;
  Sensor *s = static_cast<Sensor *>(handle);
  std::vector<unsigned char> templateData(fpTemplate, fpTemplate + cbTemplate);
  return s->DBAdd(templateData, userId) ? 1 : 0;
}

// 1:N - Identificar huella en el cache interno
int DBIdentify(SensorHandle handle, unsigned char *fpTemplate, int cbTemplate,
               int *outUserId, int *outScore) {
  if (!handle)
    return 0;
  Sensor *s = static_cast<Sensor *>(handle);
  std::vector<unsigned char> templateData(fpTemplate, fpTemplate + cbTemplate);

  int userId = 0;
  int score = 0;
  if (s->DBIdentify(templateData, userId, score)) {
    *outUserId = userId;
    *outScore = score;
    return 1; // Éxito
  }

  return 0; // No encontrado o error
}

} // fin extern "C"