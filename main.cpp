#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <conio.h>

//headers
#include "Sensor.h" 
#include "DB_Backend.hpp"
#include "TPL_utils.hpp"


using namespace std;

// Prototipo de una función simple para guardar el template (paso 2)
void guardarTemplateEnArchivo(const std::vector<unsigned char>& templateData);

int main(){

    std::cout << "--- Prueba de Sensor ZKTeco ---" << std::endl;
    
    //1. Crear instancias
    DB_Backend db("DB/huellero.db");
    Sensor miSensor;

    //2. Inicializar base de datos
    if(!db.Inicializar_DB()){
        cerr << "[!] ATENCION: No se pudo conectar la Base de datos. se continuara sin guardar en sql" << endl;
    }else{
        cout << "[+] base de datos conectada." << endl;
    }

    // 3. Inicializar el sensor
    if (!miSensor.initSensor()){
        std::cerr << "[!] FALLO CRITICO: No se pudo inicializar el sensor." << std::endl;
        std::cout << "Revisa: 1. Sensor conectado. 2. Drivers instalados. 3. DLL en la carpeta." << std::endl;
        return -1; // Termina el programa si falla
    }

    //4. usuario presiona enter y mete la huella
    std::cout << "\nSensor inicializado. Listo para capturar." << std::endl;
    std::cout << "Presiona ENTER para comenzar la captura...";
    std::cin.get(); // Espera ENTER

    // 5. Sistema para guardar el template y la imagen
    std::vector<unsigned char> huellaCapturada;
    std::vector<unsigned char> imagenCruda;
    int ancho = 0, alto = 0;
    string nombreImagen = "imagenHuella.bmp";

    // 6. Guardar Huella
    if (miSensor.capturenCreateTemplate(huellaCapturada)){
        std::cout << "\n¡¡¡EXITO!!! Huella capturada." << std::endl;
        guardarTemplateEnArchivo(huellaCapturada);
    } else{ std::cerr << "\n(!) FALLO: No se pudo capturar la huella. Intentelo de nuevo" << std::endl; }

    //7.vizualisar huella
    cout << "\n---> Presione [ENTER] para ver la imagen o [ESC] para continuar..." << endl;
    int tecla = _getch();

    switch (tecla) {
        case 13: //enter
            std::cout << "Abriendo visor de imagenes..." << std::endl;
            string comando = "start " + nombreImagen; 
            system(comando.cstr());

            //guardar en base de datos
            std::cout << "\n---> DATOS DEL ESTUDIANTE ---" << std::endl;
            std::string rut, dv, nombre, curso;

            std::cout << "RUT"; //omitible si estudiante no tiene rut
            std::cin >> rut; //generar funcion para separar rut y dv y compararlos con metodo m11
            std::cout << "Digito Varificador: ";
            std::cin << dv;
            std::cout << "Nombre del estudiante: ";
            std::cin << nombre; //funcion para guardar nombre con guion bajo y guardar como nombre y apellidos usando guion bajo como separador
            std::cout << "Curso del Estudiante: ";
            std::cin << curso;

            RequestEnrolarUsuario nuevoUsuario;
            nuevoUsuario.run_nuevo = rut;
            nuevoUsuario.dv_nuevo = dv[0];
            nuevoUsuario.nombre_nuevo = nombre;
            nuevoUsuario.curso_nuevo = curso;
            nuevoUsuario.es_pae = true;
            nuevoUsuario.template_huella = huellaCapturada;

            std::cout << "nuevo estudiante _____ Guardado correctamente dentro de la base de datos (presione enter para continuar)" << std::endl;

            if (!db.Enrolar_Estudiante_Completo(nuevoUsuario)){
                std::cerr << "\n[ERROR] No se pudo guardar en la BD." << std::endl;
                std::cerr << "Posible causa: El RUT " << rut << " ya existe." << std::endl;
                //pedirle al usuario una reincercion;
                return;
            }

            //EXITO
            std::cout << "\n>>> ¡ENROLAMIENTO COMPLETADO CON EXITO! <<<" << std::endl;
            std::cout << "El usuario " << nombre << " ha sido registrado." << std::endl;
            break;
        
        case 27:
            std::cout << "Visualizacion omitida." << std::endl;
            break;

        default:
            std::cout << "Continuando..." << std::endl;
    }

    // 5. La limpieza es automática
    std::cout << "Prueba terminada. Cerrando sensor..." << std::endl;
    return 0;
}

// --- Función de ayuda para guardar el template ---
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