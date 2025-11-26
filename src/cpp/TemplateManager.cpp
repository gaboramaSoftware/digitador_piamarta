#include "TemplateManager.hpp"
#include "libzkfp.h"

#include <iostream>
#include <string>
#include <limits>
#include <sstream>
#include <map>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <cctype> 

// Umbral de coincidencia biométrica (ajustable)
static const int MATCH_THRESHOLD = 60;

// -----------------------------------------------------------------------------
// Helpers para fecha/hora
// -----------------------------------------------------------------------------

// Devuelve la fecha actual en formato "YYYY-MM-DD"
static std::string getTodayISODate() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm local_tm{};
#ifdef _WIN32
    localtime_s(&local_tm, &t);
#else
    local_tm = *std::localtime(&t);
#endif
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local_tm);
    return std::string(buffer);
}

// Devuelve epoch en milisegundos
static std::int64_t getNowEpochMs() {
    using namespace std::chrono;
    auto now = time_point_cast<milliseconds>(system_clock::now());
    return now.time_since_epoch().count();
}

//--- guardar funciones del rut ---

// --- funciones de nombre ---
static std::string formatoNombre(std::string texto){

    if(texto.empty()) return "";
    
    bool nuevaPalabra = true;
    for(size_t i = 0; i < texto.length(); ++i){
        if (isspace(texto[i])){
            nuevaPalabra = true;
        }else if (nuevaPalabra){
            texto[i] = toupper(texto[i]);
            nuevaPalabra = false;
        }else{
            texto[i] = tolower(texto[i]);
        }
    }
    return texto;
}

//--- funciones curso --
static std::string formatoCurso(std::string texto){
    std::string limpio = "";
    for (char c : texto){
        //isalnum solo deja que numeros y letras pasen
        if(isalnum(c)) limpio += toupper(c);
    }
    return limpio;
}

// -----------------------------------------------------------------------------
// Enrolamiento
// -----------------------------------------------------------------------------

void menuEnroll(Sensor& sensor, DB_Backend& db) {
    RequestEnrolarUsuario req;

    std::cout << "\n[ENROLAMIENTO] Ingrese datos del estudiante\n";

    // -- RUT / IPE --
    std::string rawRut;
    std::cout << "RUN o IPE completo: ";
    std::cin >> rawRut;
    
    std::string cleanRut;
    for (char c : rawRut) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == 'k' || c == 'K') {
            cleanRut += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }

    // Debe tener al menos 2 caracteres: cuerpo + DV
    if (cleanRut.length() < 2) {
        std::cerr << "ERROR: RUT demasiado corto.\n";
        return;
    }

    // Separar cuerpo y DV
    char extraerDV = cleanRut.back();
    cleanRut.pop_back();               // ahora cleanRut = solo números (cuerpo)
    std::string cuerpoStr = cleanRut;  // cuerpo como string

    // Convertir cuerpo a número para el algoritmo M-11
    unsigned long cuerpo = 0;
    try {
        cuerpo = std::stoul(cuerpoStr);
    } catch (...) {
        std::cerr << "ERROR: Formato de RUT inválido (cuerpo no numérico).\n";
        return;
    }

    // Algoritmo M-11
    auto validarRut = [](unsigned long cuerpoNum, char dvIngresado) -> bool {
        unsigned long suma = 0;
        unsigned int  multi = 2;

        while (cuerpoNum > 0) {
            suma += (cuerpoNum % 10) * multi;
            cuerpoNum /= 10;
            multi++;
            if (multi > 7) multi = 2;
        }

        int resto = 11 - (suma % 11);
        char dvCalculado =
            (resto == 11) ? '0' :
            (resto == 10) ? 'K' :
            static_cast<char>('0' + resto);

        return (dvCalculado == std::toupper(static_cast<unsigned char>(dvIngresado)));
    };

    // Validar DV
    if (!validarRut(cuerpo, extraerDV)) {
        std::cerr << "(-) ERROR: RUT/IPE inválido. El dígito verificador no coincide.\n";
        std::cerr << "    Verifique si el RUT es correcto.\n";
        return;
    }

    // Asignar a la estructura del request
    req.run_nuevo = cuerpoStr;   // el RUN se guarda como string (solo cuerpo)
    req.dv_nuevo  = extraerDV;   // DV ya validado

    // ---- Nombre ----

    // limpiar resto de línea previa
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string nombreTemp, paternoTemp, maternoTemp;

    std::cout << "Nombres del alumno: ";
    std::getline(std::cin, nombreTemp);
    nombreTemp = formatoNombre(nombreTemp);

    std::cout << "Apellido Paterno del alumno (en caso de no tener, dejar en blanco): ";
    std::getline(std::cin, paternoTemp);
    paternoTemp = formatoNombre(paternoTemp);

    std::cout << "Apellido Materno del alumno (en caso de no tener, dejar en blanco): ";
    std::getline(std::cin, maternoTemp);
    maternoTemp = formatoNombre(maternoTemp);

    // Construir nombre completo
    req.nombre_nuevo = nombreTemp;
    if (!paternoTemp.empty()) req.nombre_nuevo += " " + paternoTemp;
    if (!maternoTemp.empty()) req.nombre_nuevo += " " + maternoTemp;

    // ---- Curso ----
    std::cout << "Curso (ej: 1BA, 2MB): ";
    std::getline(std::cin, req.curso_nuevo);
    req.curso_nuevo = formatoCurso(req.curso_nuevo);
    std::cout << "Curso normalizado: " << req.curso_nuevo << "\n";

    // ---- Captura de huella ----
    std::vector<unsigned char> tplUchar;
    std::cout << "\n[ENROLAMIENTO] Captura de huella para RUN: "
              << req.run_nuevo << "-" << req.dv_nuevo << "\n";

    if (!sensor.capturenCreateTemplate(tplUchar)) {
        std::cerr << "(-) No se pudo capturar huella. Enrolamiento cancelado.\n";
        return;
    }

    // Convertir vector<unsigned char> -> vector<uint8_t>
    req.template_huella.assign(tplUchar.begin(), tplUchar.end());

    // Guardar en DB
    if (db.Enrolar_Estudiante_Completo(req)) {
        std::cout << "(+) Estudiante enrolado correctamente en la base de datos.\n";
    } else {
        std::cerr << "(-) Error al enrolar estudiante en la base de datos.\n";
    }
}


// -----------------------------------------------------------------------------
// Verificación: capturar huella, buscar mejor match contra todos los templates
// -----------------------------------------------------------------------------

// Función interna: identifica a un estudiante por huella.
// Retorna true si encontró un match con score >= MATCH_THRESHOLD.
static bool identifyByFingerprint(
    Sensor& sensor,
    DB_Backend& db,
    std::string& outRun,
    int& outBestScore
) {
    // 1) Capturar huella actual
    std::vector<unsigned char> capturedTplUchar;

    std::cout << "\n[VERIFICACIÓN] Coloque su dedo en el sensor.\n";
    if (!sensor.capturenCreateTemplate(capturedTplUchar)) {
        std::cerr << "(-) No se pudo capturar huella para verificación.\n";
        return false;
    }

    std::vector<uint8_t> capturedTpl(capturedTplUchar.begin(), capturedTplUchar.end());

    // 2) Cargar todos los templates desde la DB
    std::map<std::string, std::vector<uint8_t>> templatesMap;
    if (!db.Obtener_Todos_Templates(templatesMap)) {
        std::cerr << "(-) No se pudieron cargar templates desde la DB.\n";
        return false;
    }

    // 3) Buscar el mejor match
    int bestScore = -1;
    std::string bestRun;

    std::cout << "[INFO] Comparando contra " << templatesMap.size()
              << " templates enrolados...\n";

    for (auto& [run, tpl] : templatesMap) {
        // Convertimos a vector<unsigned char> para usar Sensor::matchTemplate
        std::vector<unsigned char> tplUchar(tpl.begin(), tpl.end());
        int score = sensor.matchTemplate(capturedTplUchar, tplUchar);
        if (score < 0) {
            continue; // error en match, lo ignoramos
        }

        std::cout << "  - RUN " << run << " -> score: " << score << "\n";

        if (score > bestScore) {
            bestScore = score;
            bestRun   = run;
        }
    }

    if (bestScore >= MATCH_THRESHOLD && !bestRun.empty()) {
        std::cout << "(+) Huella verificada. Mejor coincidencia RUN: "
                  << bestRun << " (score = " << bestScore << ")\n";
        outRun       = bestRun;
        outBestScore = bestScore;
        return true;
    }

    std::cerr << "(-) No se encontró un alumno con score >= "
              << MATCH_THRESHOLD << ". Mejor score fue: "
              << bestScore << "\n";
    return false;
}

void menuVerify(Sensor& sensor, DB_Backend& db) {
    std::string run;
    int bestScore = -1;

    if (!identifyByFingerprint(sensor, db, run, bestScore)) {
        std::cerr << "(-) Verificación fallida.\n";
        return;
    }

    PerfilEstudiante perfil;
    RolUsuario rol = db.Obtener_Perfil_Por_Run(run, perfil);

    if (rol == RolUsuario::Invalido) {
        std::cerr << "(-) El RUN " << run << " no existe en la base de datos.\n";
        return;
    }

    std::cout << "\n[RESULTADO VERIFICACIÓN]\n";
    std::cout << "RUN   : " << perfil.run_id << "-" << perfil.dv << "\n";
    std::cout << "Nombre: " << perfil.nombre_completo << "\n";
    std::cout << "Curso : " << perfil.curso << "\n";
    std::cout << "Score : " << bestScore << "\n";
}

// -----------------------------------------------------------------------------
// Ticket: verificación + lógica de ración (no doble) + registro
// -----------------------------------------------------------------------------

void menuProcessTicket(Sensor& sensor, DB_Backend& db) {
    std::string run;
    int bestScore = -1;

    std::cout << "\n[TICKET] Procesamiento de ticket por huella.\n";

    // 1) Identificación por huella
    if (!identifyByFingerprint(sensor, db, run, bestScore)) {
        std::cerr << "(-) No se pudo procesar ticket: huella no reconocida.\n";
        return;
    }

    PerfilEstudiante perfil;
    RolUsuario rol = db.Obtener_Perfil_Por_Run(run, perfil);

    if (rol == RolUsuario::Invalido) {
        std::cerr << "(-) El RUN " << run << " no existe en la base de datos.\n";
        return;
    }

    // 2) Configuración global (tipo de ración, terminal, etc.)
    auto cfgOpt = db.Obtener_Configuracion();
    if (!cfgOpt.has_value()) {
        std::cerr << "(-) No hay configuración global disponible.\n";
        return;
    }
    Configuracion cfg = cfgOpt.value();

    std::string fecha = getTodayISODate();

    // 3) Verificar doble ración
    if (db.Verificar_Racion_Doble(run, fecha, cfg.tipo_racion)) {
        std::cerr << "(-) Registro rechazado: el estudiante ya recibió esta ración hoy.\n";
        return;
    }

    // 4) Construir registro de ración
    RegistroRacion reg;
    reg.id_estudiante   = run;
    reg.fecha_servicio  = fecha;
    reg.tipo_racion     = cfg.tipo_racion;
    reg.id_terminal     = cfg.id_terminal;
    reg.hora_evento     = getNowEpochMs();
    reg.estado_registro = EstadoRegistro::PENDIENTE;

    if (!db.Guardar_Registro_Racion(reg)) {
        std::cerr << "(-) Error al guardar registro de ración.\n";
        return;
    }

    // 5) "Ticket" en consola
    std::cout << "###############################################\n";
    std::cout << "  TICKET GENERADO\n";
    std::cout << "  RUN   : " << perfil.run_id << "-" << perfil.dv << "\n";
    std::cout << "  Nombre: " << perfil.nombre_completo << "\n";
    std::cout << "  Curso : " << perfil.curso << "\n";
    std::cout << "  Fecha : " << fecha << "\n";
    std::cout << "  Ración: "
              << (cfg.tipo_racion == TipoRacion::Desayuno ? "Desayuno" :
                  cfg.tipo_racion == TipoRacion::Almuerzo ? "Almuerzo" :
                  "N/A")
              << "\n";
    std::cout << "###############################################\n";
}