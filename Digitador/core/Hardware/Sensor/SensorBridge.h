#pragma once

#ifdef __cplusplus
extern "C"{
#endif

    //Creamos puntero SnesorHandle, par guardar funciones del sensor
    typedef void* SensorHandle;

    //funcion para crear y gestionar un sensor
    SensorHandle CreateSensor();
    //destructor de sensor
    void DestroySensor(SensorHandle handle);
    int InitSensor(SensorHandle handle);
    int AcquireFingerprint(SensorHandle handle, unsigned char* outBuffer, int* outSize);
    int MatchTemplates(SensorHandle handle, const unsigned char* tpl1, int size1, const unsigned char* tpl2, int size2);
    
    // 1:N Matching bridge
    int DBAdd(SensorHandle handle, int userId, unsigned char* fpTemplate, int cbTemplate);
    int DBIdentify(SensorHandle handle, unsigned char* fpTemplate, int cbTemplate, int* outUserId, int* outScore);

#ifdef __cplusplus
}
#endif