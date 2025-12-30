// SensorWorker.cpp
#include "SensorWorker.h"
#include <iostream>

SensorWorker::SensorWorker() 
    : m_sensor(nullptr)
    , m_running(false)
    , m_sensorReady(false) 
{
}

SensorWorker::~SensorWorker() {
    stop();
}

bool SensorWorker::start() {
    if (m_running.load()) {
        std::cout << "[SensorWorker] Ya está corriendo." << std::endl;
        return true;
    }

    std::cout << "[SensorWorker] Iniciando worker de sensor..." << std::endl;
    
    m_running.store(true);
    m_workerThread = std::thread(&SensorWorker::workerLoop, this);
    
    // Esperar un poco a que el sensor se inicialice
    for (int i = 0; i < 50; i++) { // Máximo 5 segundos
        if (m_sensorReady.load()) {
            std::cout << "[SensorWorker] (+) Sensor listo!" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!m_sensorReady.load()) {
        std::cerr << "[SensorWorker] (-) Sensor no pudo inicializarse." << std::endl;
        // El worker sigue corriendo pero sin sensor
        return false;
    }
    
    return true;
}

void SensorWorker::stop() {
    if (!m_running.load()) {
        return;
    }

    std::cout << "[SensorWorker] Deteniendo worker..." << std::endl;

    // Encolar comando de STOP
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        auto cmd = std::make_unique<Command>();
        cmd->type = CommandType::STOP;
        m_commandQueue.push(std::move(cmd));
    }
    m_queueCV.notify_one();

    // Esperar a que el hilo termine
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_running.store(false);
    m_sensorReady.store(false);
    
    std::cout << "[SensorWorker] Worker detenido." << std::endl;
}

void SensorWorker::workerLoop() {
    std::cout << "[SensorWorker] Hilo iniciado. Inicializando sensor..." << std::endl;
    
    // Crear e inicializar el sensor EN ESTE HILO
    m_sensor = std::make_unique<Sensor>();
    
    if (m_sensor->initSensor()) {
        m_sensorReady.store(true);
        m_sensor->setTimeout(3000);
        m_sensor->setPollInterval(100);
        std::cout << "[SensorWorker] (+) Sensor inicializado correctamente." << std::endl;
    } else {
        std::cerr << "[SensorWorker] (-) Error al inicializar sensor." << std::endl;
        m_sensorReady.store(false);
    }

    // Loop principal - procesar comandos
    while (m_running.load()) {
        std::unique_ptr<Command> cmd;
        
        // Esperar por un comando
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return !m_commandQueue.empty() || !m_running.load();
            });
            
            if (!m_running.load() && m_commandQueue.empty()) {
                break;
            }
            
            if (!m_commandQueue.empty()) {
                cmd = std::move(m_commandQueue.front());
                m_commandQueue.pop();
            }
        }
        
        if (!cmd) continue;
        
        // Procesar el comando
        switch (cmd->type) {
            case CommandType::CAPTURE:
                processCapture(*cmd);
                break;
                
            case CommandType::MATCH:
                processMatch(*cmd);
                break;
                
            case CommandType::STOP:
                m_running.store(false);
                break;
        }
    }

    // Cerrar sensor antes de salir
    if (m_sensor) {
        std::cout << "[SensorWorker] Cerrando sensor..." << std::endl;
        m_sensor->closeSensor();
        m_sensor.reset();
    }
    
    m_sensorReady.store(false);
    std::cout << "[SensorWorker] Hilo terminado." << std::endl;
}

void SensorWorker::processCapture(Command& cmd) {
    CaptureResult result;
    
    if (!m_sensorReady.load() || !m_sensor) {
        result.status = VerifyResult::NOT_INITIALIZED;
        result.errorMessage = "Sensor no inicializado";
        cmd.capturePromise.set_value(std::move(result));
        return;
    }
    
    // Configurar timeout para esta captura
    m_sensor->setTimeout(cmd.timeoutMs);
    
    // Intentar capturar
    std::vector<unsigned char> templateData;
    bool success = m_sensor->capturenCreateTemplate(templateData);
    
    if (success && !templateData.empty()) {
        result.status = VerifyResult::SUCCESS;
        result.templateData = std::move(templateData);
    } else {
        result.status = VerifyResult::NO_FINGER;
        result.errorMessage = "No se detectó huella (timeout)";
    }
    
    cmd.capturePromise.set_value(std::move(result));
}

void SensorWorker::processMatch(Command& cmd) {
    int score = -1;
    
    if (!m_sensorReady.load() || !m_sensor) {
        cmd.matchPromise.set_value(-1);
        return;
    }
    
    score = m_sensor->matchTemplate(cmd.template1, cmd.template2);
    cmd.matchPromise.set_value(score);
}

std::future<CaptureResult> SensorWorker::requestCapture(int timeoutMs) {
    auto cmd = std::make_unique<Command>();
    cmd->type = CommandType::CAPTURE;
    cmd->timeoutMs = timeoutMs;
    
    auto future = cmd->capturePromise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_commandQueue.push(std::move(cmd));
    }
    m_queueCV.notify_one();
    
    return future;
}

std::future<int> SensorWorker::requestMatch(
    const std::vector<unsigned char>& template1,
    const std::vector<unsigned char>& template2) 
{
    auto cmd = std::make_unique<Command>();
    cmd->type = CommandType::MATCH;
    cmd->template1 = template1;
    cmd->template2 = template2;
    
    auto future = cmd->matchPromise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_commandQueue.push(std::move(cmd));
    }
    m_queueCV.notify_one();
    
    return future;
}

CaptureResult SensorWorker::captureBlocking(int timeoutMs) {
    if (!m_running.load()) {
        CaptureResult result;
        result.status = VerifyResult::NOT_INITIALIZED;
        result.errorMessage = "Worker no está corriendo";
        return result;
    }
    
    auto future = requestCapture(timeoutMs);
    return future.get(); // Bloquea hasta obtener resultado
}

int SensorWorker::matchBlocking(
    const std::vector<unsigned char>& template1,
    const std::vector<unsigned char>& template2) 
{
    if (!m_running.load()) {
        return -1;
    }
    
    auto future = requestMatch(template1, template2);
    return future.get(); // Bloquea hasta obtener resultado
}