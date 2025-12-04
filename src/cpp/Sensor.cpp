// sensor.cpp

#include "sensor.h"

#include <chrono>
#include <iostream>
#include <thread>


// Si no lo tienes definido en otro lado, define el tamaño máximo de template
#ifndef MAX_TEMPLATE_SIZE
#define MAX_TEMPLATE_SIZE 2048 // Ajusta si tu SDK indica otro valor
#endif

// Constructor
Sensor::Sensor()
    : m_timeoutMs(DEFAULT_TIMEOUT_MS),
      m_pollIntervalMs(DEFAULT_POLL_INTERVAL_MS), m_deviceHandle(nullptr),
      m_dbCacheHandle(nullptr), m_imageBuffer(), m_imageWidth(0),
      m_imageHeight(0), m_isInitialized(false) {}

// Destructor
Sensor::~Sensor() { closeSensor(); }

// -----------------------------------------------------------------------------
// Inicializar sensor
// -----------------------------------------------------------------------------
bool Sensor::initSensor() {
  if (m_isInitialized) {
    return true;
  }

  // 1) Inicializar SDK
  int ret = ZKFPM_Init();
  if (ret != ZKFP_ERR_OK) {
    std::cerr << "(-) Error al inicializar ZKFP, código: " << ret << std::endl;
    return false;
  }

  // 2) Ver cuántos dispositivos hay
  int devCount = ZKFPM_GetDeviceCount();
  if (devCount <= 0) {
    std::cerr << "(-) No se encontraron dispositivos de huella." << std::endl;
    ZKFPM_Terminate();
    return false;
  }

  // 3) Abrir el primer dispositivo
  m_deviceHandle = ZKFPM_OpenDevice(0);
  if (!m_deviceHandle) {
    std::cerr << "(-) No se pudo abrir el dispositivo 0." << std::endl;
    ZKFPM_Terminate();
    return false;
  }

  // 4) Intentar obtener ancho/alto/DPI
  int width = 0;
  int height = 0;
  int dpi = 0;

  ret = ZKFPM_GetCaptureParamsEx(m_deviceHandle, &width, &height, &dpi);
  if (ret != ZKFP_ERR_OK) {
    std::cerr << "(-) Error al obtener ancho de imagen, código: " << ret
              << std::endl;
    std::cerr << "    Usando valores por defecto 250x360 @500dpi.\n";

    // Valores por defecto razonables
    width = 250;
    height = 360;
    dpi = 500;
  }

  m_imageWidth = width;
  m_imageHeight = height;

  // Reservar buffer como vector
  const auto pixelCount =
      static_cast<size_t>(m_imageWidth) * static_cast<size_t>(m_imageHeight);
  m_imageBuffer.resize(pixelCount);

  // 5) Inicializar DB de templates
  m_dbCacheHandle = ZKFPM_DBInit();
  if (!m_dbCacheHandle) {
    std::cerr << "(-) Error al inicializar DB de huellas." << std::endl;
    m_imageBuffer.clear();
    ZKFPM_CloseDevice(m_deviceHandle);
    m_deviceHandle = nullptr;
    ZKFPM_Terminate();
    return false;
  }

  m_isInitialized = true;
  return true;
}

// -----------------------------------------------------------------------------
// Cerrar sensor
// -----------------------------------------------------------------------------
bool Sensor::closeSensor() {
  if (!m_isInitialized) {
    return true;
  }

  // Liberar DB de templates
  if (m_dbCacheHandle) {
    ZKFPM_DBFree(m_dbCacheHandle);
    m_dbCacheHandle = nullptr;
  }

  // Cerrar dispositivo
  if (m_deviceHandle) {
    ZKFPM_CloseDevice(m_deviceHandle);
    m_deviceHandle = nullptr;
  }

  // Limpiar buffer de imagen
  m_imageBuffer.clear();
  m_imageWidth = 0;
  m_imageHeight = 0;

  // Terminar SDK
  ZKFPM_Terminate();

  m_isInitialized = false;
  return true;
}

// -----------------------------------------------------------------------------
// Capturar huella y crear template
// -----------------------------------------------------------------------------
bool Sensor::capturenCreateTemplate(std::vector<unsigned char> &templateData) {
  if (!m_isInitialized) {
    std::cerr << "(-) Error: el sensor no está inicializado." << std::endl;
    return false;
  }

  if (m_imageBuffer.empty()) {
    std::cerr << "(-) Error: buffer de imagen no inicializado." << std::endl;
    return false;
  }

  std::cout << ">>> COLOQUE SU DEDO EN EL SENSOR <<<" << std::endl;

  unsigned char tempTemplateBuffer[MAX_TEMPLATE_SIZE];

  // Tiempo de espera y deadline
  auto startWait = std::chrono::steady_clock::now();
  auto deadline = startWait + std::chrono::milliseconds(m_timeoutMs);

  int ret = ZKFP_ERR_FAIL;
  bool success = false;
  unsigned int templateSize = 0;

  while (std::chrono::steady_clock::now() < deadline) {
    templateSize = MAX_TEMPLATE_SIZE; // SIEMPRE resetear antes de llamar al SDK

    unsigned int imageSize = static_cast<unsigned int>(m_imageBuffer.size());

    ret = ZKFPM_AcquireFingerprint(
        m_deviceHandle,
        m_imageBuffer.data(), // <--- ojo: .data(), no el vector entero
        imageSize, tempTemplateBuffer, &templateSize);

    if (ret == ZKFP_ERR_OK) {
      success = true;
      break;
    }

#ifdef ZKFP_ERR_NO_FINGER
    if (ret == ZKFP_ERR_NO_FINGER) {
      // No hay dedo: feedback + espera
      std::cout << "." << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
      continue;
    } else {
      // Otro error distinto a OK/NO_FINGER: abortamos
      std::cerr << "\n(-) Error al capturar huella, código: " << ret
                << std::endl;
      return false;
    }
#else
    // Si NO tenemos NO_FINGER en el SDK, tratamos todo != OK como "no hay dedo
    // / mala lectura"
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
    continue;
#endif
  }

  if (!success) {
    std::cout << std::endl;
    std::cerr << "(-) Tiempo agotado: no se detectó dedo en el sensor."
              << std::endl;
    return false;
  }

  // Momento en que se capturó una huella válida
  auto captureTime = std::chrono::steady_clock::now();

  // Copiar el template al vector de salida
  templateData.assign(tempTemplateBuffer, tempTemplateBuffer + templateSize);

  std::cout << "\n(+) Huella capturada. Tamaño del template: " << templateSize
            << " bytes." << std::endl;

  // Pequeño delay mínimo para UX
  constexpr auto MIN_DELAY = std::chrono::milliseconds(500); // 0.5 segundos
  std::cout << "Procesando, por favor espere..." << std::endl;

  auto now = std::chrono::steady_clock::now();
  if (now < captureTime + MIN_DELAY) {
    auto remaining = (captureTime + MIN_DELAY) - now;
    std::this_thread::sleep_for(remaining);
  }

  return true;
}

// -----------------------------------------------------------------------------
// Obtener la última imagen capturada (para debug / mostrar huella en pantalla)
// -----------------------------------------------------------------------------
bool Sensor::captureLastTemplate(std::vector<unsigned char> &imgOut, int &width,
                                 int &height) {
  if (!m_isInitialized || m_imageBuffer.empty()) {
    std::cerr
        << "(-) No hay imagen disponible o el sensor no está inicializado."
        << std::endl;
    return false;
  }

  width = m_imageWidth;
  height = m_imageHeight;

  imgOut = m_imageBuffer; // copia completa
  return true;
}

// -----------------------------------------------------------------------------
// Comparar dos templates y retornar el score de coincidencia
// -----------------------------------------------------------------------------
int Sensor::matchTemplate(const std::vector<unsigned char> &template1,
                          const std::vector<unsigned char> &template2) {
  if (!m_isInitialized || !m_dbCacheHandle) {
    std::cerr << "(-) matchTemplate: sensor o DB no inicializados."
              << std::endl;
    return -1;
  }

  if (template1.empty() || template2.empty()) {
    std::cerr << "(-) matchTemplate: uno de los templates está vacío."
              << std::endl;
    return -1;
  }

  unsigned int size1 = static_cast<unsigned int>(template1.size());
  unsigned int size2 = static_cast<unsigned int>(template2.size());

  unsigned char *tpl1 = const_cast<unsigned char *>(template1.data());
  unsigned char *tpl2 = const_cast<unsigned char *>(template2.data());

  int score = ZKFPM_DBMatch(m_dbCacheHandle, tpl1, size1, tpl2, size2);

  if (score < 0) {
    std::cerr << "(-) Error en DBMatch, código: " << score << std::endl;
  }

  return score; // "score" o "porcentaje" según el SDK
}

// -----------------------------------------------------------------------------
// Intentar capturar huella inmediatamente (Non-blocking)
// -----------------------------------------------------------------------------
bool Sensor::acquireFingerprintImmediate(
    std::vector<unsigned char> &templateData) {
  if (!m_isInitialized)
    return false;
  if (m_imageBuffer.empty())
    return false;

  unsigned char tempTemplateBuffer[MAX_TEMPLATE_SIZE];
  unsigned int templateSize = MAX_TEMPLATE_SIZE;
  unsigned int imageSize = static_cast<unsigned int>(m_imageBuffer.size());

  int ret =
      ZKFPM_AcquireFingerprint(m_deviceHandle, m_imageBuffer.data(), imageSize,
                               tempTemplateBuffer, &templateSize);

  if (ret == ZKFP_ERR_OK) {
    // Éxito: copiar template
    templateData.assign(tempTemplateBuffer, tempTemplateBuffer + templateSize);
    return true;
  }

  // Si es NO_FINGER o cualquier otro error, retornamos false inmediatamente.
  // No imprimimos nada para no ensuciar el log en el loop rápido.
  return false;
}
