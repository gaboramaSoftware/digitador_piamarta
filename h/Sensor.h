// sensor.h
#pragma once // evita duplicados

#include <windows.h>
#include <vector>
#include <string>

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
    bool capturenCreateTemplate(std::vector<unsigned char>& templateData);

    // obtener la última captura exitosa (imagen)
    bool captureLastTemplate(std::vector<unsigned char>& imgOut, int& width, int& height);

    // comparar templates, retorna porcentaje de coincidencia / score
    int matchTemplate(const std::vector<unsigned char>& template1,
                      const std::vector<unsigned char>& template2);

    // verificar que el sensor esté listo
    bool isInitialized() const { return m_isInitialized; }

    // configuraciones de tiempo de captura de imagen (en milisegundos)
    void setTimeout(int ms)       { m_timeoutMs = ms; }
    void setPollInterval(int ms)  { m_pollIntervalMs = ms; }

private:
    // configuración por defecto (constantes internas)
    static constexpr int DEFAULT_TIMEOUT_MS      = 4000; // 4 segundos
    static constexpr int DEFAULT_POLL_INTERVAL_MS = 200; // 200 ms

    // configuración actual (se puede cambiar con setters)
    int m_timeoutMs      = DEFAULT_TIMEOUT_MS;
    int m_pollIntervalMs = DEFAULT_POLL_INTERVAL_MS;

    // manejos / handles del SDK
    HANDLE m_deviceHandle;
    HANDLE m_dbCacheHandle;

    // buffer interno donde el SDK coloca la imagen
    unsigned char* m_imageBuffer;
    int            m_imageWidth;
    int            m_imageHeight;
    unsigned int   m_imageBufferSize;

    bool m_isInitialized;
};
