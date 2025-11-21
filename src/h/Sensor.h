// sensor.h
// Template.h  (usable desde C y C++)

#pragma once // evita duplicados

#include <windows.h>
#include <vector>
#include <string>

// zkteco sdk
#include "libzkfp.h"
#include "libzkfperrdef.h"

class Sensor {
public:
    // constructor/destructor
    Sensor();
    ~Sensor();

    // iniciar/cerrar sensor
    bool initSensor();
    bool closeSensor();

    // capturar huella -> crear template
    bool capturenCreateTemplate(std::vector<unsigned char>& templateData);

    // match de huella
    int matchTemplate(const std::vector<unsigned char>& template1,
                      const std::vector<unsigned char>& template2);

    // verificar si el sensor está listo
    bool isInitialized() const { return m_isInitialized; }

private:
    // manejos/handles del SDK
    HANDLE m_deviceHandle;
    HANDLE m_dbCacheHandle;
    unsigned char* m_imageBuffer;
    int m_imageWidth;
    int m_imageHeight;
    unsigned int m_imageBufferSize;
    bool m_isInitialized; // Esta es la variable
};
