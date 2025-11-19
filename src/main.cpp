#include <iostream>
#include <vector>
#include "Sensor.h" 
#include "DB_Backend.hpp"

// Prototipo de una función simple para guardar el template (paso 2)
void guardarTemplateEnArchivo(const std::vector<unsigned char>& templateData);

int main(){

    std::cout << "--- Prueba de Sensor ZKTeco ---" << std::endl;
    
    // 1. Crear una instancia de tu clase
    Sensor miSensor;

    // 2. Inicializar el sensor
    if (!miSensor.initSensor()){
        std::cerr << "(!) FALLO CRITICO: No se pudo inicializar el sensor." << std::endl;
        std::cout << "Revisa: 1. Sensor conectado. 2. Drivers instalados. 3. DLL en la carpeta." << std::endl;
        return -1; // Termina el programa si falla
    }

    std::cout << "\nSensor inicializado. Listo para capturar." << std::endl;
    std::cout << "Presiona ENTER para comenzar la captura...";
    std::cin.get(); // Espera a que presiones Enter

    // 3. Crear un vector para guardar el template
    std::vector<unsigned char> huellaCapturada;

    // 4. Llamar a tu función de captura
    if (miSensor.capturenCreateTemplate(huellaCapturada)){
        std::cout << "\n¡¡¡EXITO!!! Huella capturada." << std::endl;

        // (Opcional pero recomendado) Guardar la huella para pruebas
        guardarTemplateEnArchivo(huellaCapturada);
    } 
    else{
        std::cerr << "\n(!) FALLO: No se pudo capturar la huella." << std::endl;
    }

    // 5. La limpieza es automática
    std::cout << "Prueba terminada. Cerrando sensor..." << std::endl;
    return 0;
}

// --- Función de ayuda para guardar el template ---
#include <fstream> // Para escribir archivos
void guardarTemplateEnArchivo(const std::vector<unsigned char>& templateData){
    std::ofstream archivoSalida("mi_huella.tpl", std::ios::binary);
    if (!archivoSalida) {
        std::cerr << "No se pudo crear el archivo 'mi_huella.tpl'" << std::endl;
        return;
    }

    // Escribe los bytes del vector al archivo
    archivoSalida.write(reinterpret_cast<const char*>(templateData.data()), templateData.size());
    archivoSalida.close();
    std::cout << "Template guardado como 'mi_huella.tpl'" << std::endl;

}