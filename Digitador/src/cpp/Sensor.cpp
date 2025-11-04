//sensor.cpp

//header
#include "Sensor.h"
//standard libs
#include <iostream>
#include <cstdio>
#include <vector>
#include <new> //prar los std::notthrow


//abrir huellero
Sensor::Sensor()
    : m_deviceHandle(nullptr),
      m_dbCacheHandle(nullptr),
      m_imageBuffer(nullptr),
      m_imageWidth(0),
      m_imageHeight(0),
      m_imageBufferSize(0),
      m_isInitialized(false)
{
    // El cuerpo AHORA SÍ puede estar vacío
}

//cerrar huellero
Sensor::~Sensor(){ this->closeSensor(); }

//iniciar sensor
bool Sensor::initSensor()
{
    if (ZKFPM_Init() != ZKFP_ERR_OK) {
        std::cerr << "(-) Error: no se pudo inicializar la biblioteca de ZKTeco." << std::endl;
        return false;
    }
    
    //ABRIR DISPOSITIVO
    m_deviceHandle = ZKFPM_OpenDevice(0);
    if (m_deviceHandle == nullptr){
        std::cerr << "(-) Error: no se pudo conectar con el sensor (OpenDevice fallo)." << std::endl;
        ZKFPM_Terminate(); //limpieza
        return false;
    }

    //CREAR MOTOR DE BASE DE DATOS PARA COMPARAR
    m_dbCacheHandle = ZKFPM_DBInit(); // Usamos el alias más nuevo
    if (m_dbCacheHandle == nullptr) {
        std::cerr << "(-) Error: ZKFPM_DBInit" << std::endl;
        ZKFPM_CloseDevice(m_deviceHandle);
        ZKFPM_Terminate();
        return false;
    }

    //OBTENER PARAMETROS DE IMAGEN
    int dpi = 0;
    int ret = ZKFPM_GetCaptureParamsEx(m_deviceHandle, &m_imageWidth, &m_imageHeight, &dpi);
    if (ret != ZKFP_ERR_OK) {
        std::cerr << "(-) Error: ZKFPM_GetCaptureParamsEx" << std::endl;
        closeSensor();
        return false;
    }

    //ASIGNAR BUFFER DE IMAGEN
    m_imageBufferSize = m_imageWidth * m_imageHeight;
    m_imageBuffer = new unsigned char[m_imageBufferSize];
    if (m_imageBuffer == nullptr) {
        std::cerr << "(-) Error: no se pudo asignar memoria para el buffer de imagen." << std::endl;
        closeSensor(); //limpieza
        return false;
    }

    //éxito
    std::cout << "(+) Sensor ZKTeco inicializado correctamente." << std::endl;
    m_isInitialized = true;
    return true; 
}

//cerrar sensor
bool Sensor::closeSensor(){
    try{
        if (m_imageBuffer != nullptr){
            delete[] m_imageBuffer;
            m_imageBuffer = nullptr;
        }
        if (m_dbCacheHandle != nullptr){
            ZKFPM_DBFree(m_dbCacheHandle);
            m_dbCacheHandle = nullptr;
        }
        if (m_deviceHandle != nullptr){
            ZKFPM_CloseDevice(m_deviceHandle);
            m_deviceHandle = nullptr;
        }
        if (m_isInitialized){
            ZKFPM_Terminate();
            m_isInitialized = false;
        }
        return true; //Exito
    }
    catch (...){
        std::cerr << "(-) Error: Algo paso al cerrar el sensor." << std::endl;  
        return false;
    }
}

bool Sensor::capturenCreateTemplate(std::vector<unsigned char>& templateData)
{
    //varificar estado del sensor
    if (!m_isInitialized){
        std::cerr << "(-) Error: el sensor no esta inicializado." << std::endl;
        return false;
    }
    
    std::cout << "Por favor coloque el dedo en el sensor..." << std::endl;

    //buffer temporal para el template
    unsigned char tempTemplateBuffer[MAX_TEMPLATE_SIZE]; 
    unsigned int templateSize = MAX_TEMPLATE_SIZE;

    //Capturar huella y crear template
    int ret = ZKFPM_AcquireFingerprint(m_deviceHandle,
                                       m_imageBuffer, 
                                       m_imageBufferSize, 
                                       tempTemplateBuffer, 
                                       &templateSize);
    
    if (ret != ZKFP_ERR_OK){
        std::cerr << "(-) Error: fallo al capturar la huella dactilar, codigo" << ret << std::endl;
        return false;
    }

    //Copiamos los datos al vector de salida
    templateData.assign(tempTemplateBuffer, tempTemplateBuffer + templateSize);
    std::cout << "(+) Huella capturada y template creado correctamente. tamaño: " << templateSize << " bytes." << std::endl;
    return true; //exito
}

int Sensor::matchTemplate(const std::vector<unsigned char>& template1, const std::vector<unsigned char>& template2){
    if (!m_isInitialized){
        std::cerr << "(-) Error: No se pueden hacer comparaciones de huella." << std::endl;
        return 0; //sin coincidencia
    }

    //funcion de comparacion
    int score = ZKFPM_DBMatch(m_dbCacheHandle,
                            (unsigned char*)template1.data(),
                            (unsigned int) template1.size(),
                            (unsigned char*)template2.data(),
                            (unsigned char*)template2.data(),
                            (unsigned int) template2.size());

    if (score < 0){
        std::cerr << "(-) Error: fallo en la comparación de huellas, código: " << score << std::endl;
        return 0; //sin coincidencia
    }                            

return score; //devolver % de coincidencia    
}
