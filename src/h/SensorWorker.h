// SensorWorker.h
// Maneja el sensor de huellas en un hilo dedicado con cola de comandos
#pragma once

#include "sensor.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <future>
#include <memory>

// Tipos de resultado de verificación
enum class VerifyResult {
    SUCCESS,           // Huella capturada exitosamente
    NO_FINGER,         // No se detectó dedo (timeout)
    SENSOR_ERROR,      // Error del sensor
    SENSOR_BUSY,       // Sensor ocupado con otra operación
    NOT_INITIALIZED    // Sensor no inicializado
};

// Estructura para el resultado de una captura
struct CaptureResult {
    VerifyResult status;
    std::vector<unsigned char> templateData;
    std::string errorMessage;
};

// Clase que encapsula el sensor en un hilo dedicado
class SensorWorker {
public:
    SensorWorker();
    ~SensorWorker();

    // Iniciar el worker (arranca el hilo y el sensor)
    bool start();
    
    // Detener el worker (cierra sensor y termina hilo)
    void stop();
    
    // ¿Está el sensor listo?
    bool isReady() const { return m_sensorReady.load(); }
    
    // ¿Está el worker corriendo?
    bool isRunning() const { return m_running.load(); }

    // Solicitar captura de huella (NO BLOQUEANTE)
    // Retorna un future que se completará cuando la captura termine
    std::future<CaptureResult> requestCapture(int timeoutMs = 3000);
    
    // Solicitar match entre dos templates (NO BLOQUEANTE)
    std::future<int> requestMatch(
        const std::vector<unsigned char>& template1,
        const std::vector<unsigned char>& template2
    );

    // Versión BLOQUEANTE simplificada para captura
    // Útil para el endpoint REST
    CaptureResult captureBlocking(int timeoutMs = 3000);
    
    // Versión BLOQUEANTE para match
    int matchBlocking(
        const std::vector<unsigned char>& template1,
        const std::vector<unsigned char>& template2
    );

private:
    // Tipos de comando que puede procesar el worker
    enum class CommandType {
        CAPTURE,
        MATCH,
        STOP
    };

    // Estructura de comando genérico
    struct Command {
        CommandType type;
        int timeoutMs;
        std::vector<unsigned char> template1;
        std::vector<unsigned char> template2;
        std::promise<CaptureResult> capturePromise;
        std::promise<int> matchPromise;
    };

    // El hilo principal del worker
    void workerLoop();
    
    // Procesar comandos específicos
    void processCapture(Command& cmd);
    void processMatch(Command& cmd);

    // El sensor (único, manejado solo por este hilo)
    std::unique_ptr<Sensor> m_sensor;
    
    // Hilo del worker
    std::thread m_workerThread;
    
    // Cola de comandos
    std::queue<std::unique_ptr<Command>> m_commandQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    
    // Estados
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_sensorReady{false};
};