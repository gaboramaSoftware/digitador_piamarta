// sensor.h
#pragma once // evita duplicados

#include <string>
#include <vector>
#include <windows.h>

// zkteco sdk
#include "libzkfp.h"
#include "libzkfperrdef.h"

class Sensor {
public:
  // constructor / destructor
  Sensor();
  ~Sensor();

  // iniciar / cerrar sensor
  bool initSensor();
  bool closeSensor();

  // capturar huella -> crear template
  bool capturenCreateTemplate(std::vector<unsigned char> &templateData);

  // obtener la última captura exitosa (imagen)
  bool captureLastTemplate(std::vector<unsigned char> &imgOut, int &width,
                           int &height);

  // Intenta capturar una huella inmediatamente (sin esperar timeout).
  // Retorna true si capturó, false si no hay dedo o error.
  bool acquireFingerprintImmediate(std::vector<unsigned char> &templateData);

  // comparar templates, retorna porcentaje de coincidencia / score
  int matchTemplate(const std::vector<unsigned char> &template1,
                    const std::vector<unsigned char> &template2);

  // verificar que el sensor esté listo
  bool isInitialized() const { return m_isInitialized; }

  // configuraciones de tiempo de captura de imagen (en milisegundos)
  void setTimeout(int ms) { m_timeoutMs = ms; }
  void setPollInterval(int ms) { m_pollIntervalMs = ms; }

private:
  // configuración por defecto (constantes internas)
  static constexpr int DEFAULT_TIMEOUT_MS = 4000;      // 4 segundos
  static constexpr int DEFAULT_POLL_INTERVAL_MS = 200; // 200 ms

  // configuración actual (se puede cambiar con setters)
  int m_timeoutMs;      // se inicializa en el .cpp
  int m_pollIntervalMs; // idem

  // handles del SDK
  HANDLE m_deviceHandle;  // dev
  HANDLE m_dbCacheHandle; // DB de templates

  // buffer interno donde el SDK coloca la imagen
  std::vector<unsigned char> m_imageBuffer; // en vez de raw pointer
  int m_imageWidth;
  int m_imageHeight;

  bool m_isInitialized;
};
