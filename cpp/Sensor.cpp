#include "sensor.h"

#include <iostream>
#include <chrono>
#include <thread>

// Si no lo tienes definido en otro lado, define el tamaño máximo de template
#ifndef MAX_TEMPLATE_SIZE
#define MAX_TEMPLATE_SIZE 2048  // Ajusta si tu SDK indica otro valor
#endif

// Constructor
Sensor::Sensor()
    : m_deviceHandle(nullptr),
      m_dbCacheHandle(nullptr),
      m_imageBuffer(nullptr),
      m_imageWidth(0),
      m_imageHeight(0),
      m_imageBufferSize(0),
      m_isInitialized(false),
      m_timeoutMs(DEFAULT_TIMEOUT_MS),
      m_pollIntervalMs(DEFAULT_POLL_INTERVAL_MS)
{
}

// Destructor
Sensor::~Sensor() {
    closeSensor();
}

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

    // -----------------------------------------------------------------
    // 4) Obtener tamaño de imagen desde el SDK
    //    OJO: esto depende de tu versión de libzkfp.
    //    Muchas demos usan ZKFPM_GetParameters con IDs 1 (width) y 2 (height).
    // -----------------------------------------------------------------

    int width  = 0;
    int height = 0;
    unsigned int size = 0;

    // Estos IDs (1 y 2) son típicos, pero revisa tu demo / documentación.
    ret = ZKFPM_GetParameters(m_deviceHandle, 1, (unsigned char*)&width, &size);
    if (ret != ZKFP_ERR_OK) {
        std::cerr << "(-) Error al obtener ancho de imagen, código: " << ret << std::endl;
        closeSensor();
        return false;
    }

    ret = ZKFPM_GetParameters(m_deviceHandle, 2, (unsigned char*)&height, &size);
    if (ret != ZKFP_ERR_OK) {
        std::cerr << "(-) Error al obtener alto de imagen, código: " << ret << std::endl;
        closeSensor();
        return false;
    }

    m_imageWidth  = width;
    m_imageHeight = height;
    m_imageBufferSize = static_cast<unsigned int>(m_imageWidth * m_imageHeight);

    // 5) Crear buffer de imagen
    m_imageBuffer = new unsigned char[m_imageBufferSize];

    // 6) Inicializar DB de templates (para match, identify, etc.)
    m_dbCacheHandle = ZKFPM_DBInit();
    if (!m_dbCacheHandle) {
        std::cerr << "(-) Error al inicializar DB de huellas." << std::endl;
        delete[] m_imageBuffer;
        m_imageBuffer = nullptr;
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

    // Liberar buffer de imagen
    if (m_imageBuffer) {
        delete[] m_imageBuffer;
        m_imageBuffer = nullptr;
    }

    // Terminar SDK
    ZKFPM_Terminate();

    m_isInitialized = false;
    return true;
}

// -----------------------------------------------------------------------------
// Capturar huella y crear template
// -----------------------------------------------------------------------------
bool Sensor::capturenCreateTemplate(std::vector<unsigned char>& templateData)
{
    if (!m_isInitialized) {
        std::cerr << "(-) Error: el sensor no está inicializado." << std::endl;
        return false;
    }

    std::cout << ">>> COLOQUE SU DEDO EN EL SENSOR <<<" << std::endl;

    unsigned char tempTemplateBuffer[MAX_TEMPLATE_SIZE];

    // Tiempo de espera y deadline
    auto startWait = std::chrono::steady_clock::now();
    auto deadline  = startWait + std::chrono::milliseconds(m_timeoutMs);

    int ret = ZKFP_ERR_FAIL;
    bool success = false;
    unsigned int templateSize = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        templateSize = MAX_TEMPLATE_SIZE; // SIEMPRE resetear antes de llamar al SDK

        ret = ZKFPM_AcquireFingerprint(
            m_deviceHandle,
            m_imageBuffer,
            m_imageBufferSize,
            tempTemplateBuffer,
            &templateSize
        );

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
            std::cerr << "\n(-) Error al capturar huella, código: " << ret << std::endl;
            return false;
        }
#else
        // Si NO tenemos NO_FINGER en el SDK, tratamos todo != OK como "no hay dedo / mala lectura"
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
        continue;
#endif
    }

    if (!success) {
        std::cout << std::endl;
        std::cerr << "(-) Tiempo agotado: no se detectó dedo en el sensor." << std::endl;
        return false;
    }

    // Momento en que se capturó una huella válida
    auto captureTime = std::chrono::steady_clock::now();

    // Copiar el template al vector de salida
    templateData.assign(tempTemplateBuffer, tempTemplateBuffer + templateSize);

    std::cout << "\n(+) Huella capturada. Tamaño del template: "
              << templateSize << " bytes." << std::endl;

    // -------------------------------------------------------------------------
    // UX: pequeño delay mínimo antes de devolver control (ej. ticket, pantalla)
    // -------------------------------------------------------------------------
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
bool Sensor::captureLastTemplate(std::vector<unsigned char>& imgOut,
                                 int& width,
                                 int& height)
{
    if (!m_isInitialized || !m_imageBuffer || m_imageBufferSize == 0) {
        std::cerr << "(-) No hay imagen disponible o el sensor no está inicializado." << std::endl;
        return false;
    }

    width  = m_imageWidth;
    height = m_imageHeight;

    imgOut.assign(m_imageBuffer, m_imageBuffer + m_imageBufferSize);
    return true;
}

// -----------------------------------------------------------------------------
// Comparar dos templates y retornar el score de coincidencia
// -----------------------------------------------------------------------------
int Sensor::matchTemplate(const std::vector<unsigned char>& template1,
                          const std::vector<unsigned char>& template2)
{
    if (!m_isInitialized || !m_dbCacheHandle) {
        std::cerr << "(-) matchTemplate: sensor o DB no inicializados." << std::endl;
        return -1;
    }

    if (template1.empty() || template2.empty()) {
        std::cerr << "(-) matchTemplate: uno de los templates está vacío." << std::endl;
        return -1;
    }

    // En muchos ejemplos de ZKTeco, ZKFPM_DBMatch:
    // - Retorna un entero con el "score" de similitud (> 0 si hay coincidencia).
    // - Retorna < 0 en caso de error.
    int score = ZKFPM_DBMatch(
        m_dbCacheHandle,
        const_cast<unsigned char*>(template1.data()),
        const_cast<unsigned char*>(template2.data())
    );

    if (score < 0) {
        std::cerr << "(-) Error en DBMatch, código: " << score << std::endl;
    }

    return score; // Puede interpretarse como "porcentaje" o "score" según el SDK
}
