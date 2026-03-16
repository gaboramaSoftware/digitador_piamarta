# Reporte de Arquitectura — Sistema Digitador Piamarta

## Diagrama General del Sistema

```mermaid
graph TD
    subgraph "ENTRY POINT"
        CMD["cmd/main.go"]
    end

    subgraph "INFRA — Capa de Infraestructura"
        MAIN["infra/main.go<br/>(Menú CLI + Orquestador)"]
        API["infra/web/ApiRestServer.go<br/>(Servidor HTTP :8080)"]
        ELECTRON["infra/ElectronTotem.go<br/>(Lanzador Electron)"]
        DBREPO["infra/DB/dbRepository.go<br/>(SQLiteUserRepository)"]
    end

    subgraph "APP — Lógica de Aplicación"
        ENROLL["app/logic/enrollUsers.go"]
        VERIFY["app/logic/VerificarUser.go"]
        TICKET_LOGIC["app/logic/menuTicket.go"]
        SHOW["app/logic/menuShowRecent.go"]
        TICKET_PRINT["app/ticket/ticket.go"]
    end

    subgraph "CORE — Dominio"
        MODELS["core/db/Models.go<br/>(Usuario, RegistroRacion, DTOs)"]
        SENSOR_GO["core/Hardware/Sensor/SensorAdapter.go<br/>(CGO Bridge)"]
        SENSOR_CPP["core/Hardware/Sensor/Sensor.cpp<br/>(SDK DigitalPersona)"]
        DB_FILE["core/db/digitador.db<br/>(SQLite)"]
    end

    subgraph "FRONTENDS"
        WEBPAGE["webpage/<br/>(Dashboard Admin)"]
        TOTEM["infra/totem/<br/>(Totem Electron)"]
    end

    CMD --> MAIN
    MAIN --> API
    MAIN --> ELECTRON
    MAIN --> ENROLL
    MAIN --> VERIFY
    MAIN --> TICKET_LOGIC
    MAIN --> SHOW
    MAIN --> DBREPO
    MAIN --> SENSOR_GO

    API --> DBREPO
    API --> SENSOR_GO
    API --> TICKET_PRINT
    DBREPO --> MODELS
    DBREPO --> DB_FILE
    SENSOR_GO --> SENSOR_CPP

    WEBPAGE -->|"HTTP fetch /api/*"| API
    TOTEM -->|"HTTP fetch /api/*"| API
    ELECTRON -->|"npx electron"| TOTEM

    ENROLL --> SENSOR_GO
    ENROLL --> DBREPO
    VERIFY --> SENSOR_GO
    VERIFY --> DBREPO
    TICKET_LOGIC --> SENSOR_GO
    TICKET_LOGIC --> DBREPO
    TICKET_LOGIC --> TICKET_PRINT
```

---

## 1. Conexiones Frontend ↔ Backend

### Dashboard Admin (`webpage/`)

| Archivo | Rol |
|---|---|
| [index.html](file:///c:/Proyectos/Pydigitador/webpage/index.html) | Estructura HTML del panel admin |
| [app.js](file:///c:/Proyectos/Pydigitador/webpage/app.js) | Lógica JS, llamadas a API |
| [style.css](file:///c:/Proyectos/Pydigitador/webpage/style.css) | Estilos del dashboard |

**¿Cómo se conecta al backend?**
El servidor Go en [ApiRestServer.go](file:///c:/Proyectos/Pydigitador/infra/web/ApiRestServer.go) sirve la carpeta `webpage/` como archivos estáticos (línea 207):
```go
mux.Handle("/", http.FileServer(http.Dir("webpage")))
```
Cuando el usuario abre `http://localhost:8080` en el navegador, Go le entrega [index.html](file:///c:/Proyectos/Pydigitador/webpage/index.html). Luego, [app.js](file:///c:/Proyectos/Pydigitador/webpage/app.js) hace llamadas [fetch()](file:///c:/Proyectos/Pydigitador/webpage/app.js#106-115) a las siguientes APIs **relativas** (sin dominio, funciona porque se sirve desde el mismo servidor):

| Endpoint JS | Método | Endpoint Go | Función en dbRepository |
|---|---|---|---|
| `/api/sensor/status` | GET | `/api/Sensor/status` | (inline, verifica `s != nil`) |
| `/api/stats` | GET | `/api/stats` | [GetStats()](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#342-352) |
| `/api/recent` | GET | `/api/recent` | [GetRecentRecords(20)](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#298-341) |
| `/api/students` | GET | `GET /api/students` | [GetAllUsers()](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#118-160) |
| `/api/students` | POST | `POST /api/students` | [AddStudent()](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#68-72) |
| `/api/students/{run}/stats` | GET | `GET /api/students/{run}/stats` | [GetStudentStats(run)](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#393-401) |
| `/api/students/{run}/history` | GET | `GET /api/students/{run}/history` | [GetStudentHistory(run)](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#353-392) |
| `/api/students/{run}` | PUT | `PUT /api/students/{run}` | [UpdateStudent()](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#73-77) |
| `/api/students/{run}` | DELETE | `DELETE /api/students/{run}` | [DeleteStudentByRun(run)](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go#78-82) |
| `/api/export/records` | GET | ⚠️ **NO IMPLEMENTADO** | — |
| `/api/export/students` | GET | ⚠️ **NO IMPLEMENTADO** | — |

> [!WARNING]
> **Inconsistencia detectada:** El JS en [app.js](file:///c:/Proyectos/Pydigitador/webpage/app.js) llama a `/api/sensor/status` (minúscula) pero el endpoint en Go está registrado como `/api/Sensor/status` (mayúscula). Esto puede funcionar o no dependiendo del SO, pero debería unificarse.

> [!WARNING]
> **Endpoints faltantes:** Los botones "Exportar a Excel" en el Dashboard llaman a `/api/export/records` y `/api/export/students`, pero estos endpoints **no están implementados** en [ApiRestServer.go](file:///c:/Proyectos/Pydigitador/infra/web/ApiRestServer.go). Harán que el botón muestre error.

---

### Totem Electron (`infra/totem/`)

| Archivo | Rol |
|---|---|
| [main.js](file:///c:/Proyectos/Pydigitador/infra/totem/main.js) | Proceso principal Electron (crea ventana) |
| [index.html](file:///c:/Proyectos/Pydigitador/infra/totem/index.html) | UI del tótem (4 pantallas: waiting, processing, approved, rejected) |
| [renderer.js](file:///c:/Proyectos/Pydigitador/infra/totem/renderer.js) | Lógica de polling y verificación de huella |

**¿Cómo se conecta al backend?**
A diferencia del Dashboard (que usa rutas relativas), el Totem se conecta por **URL absoluta**:
```javascript
const API_URL = 'http://localhost:8080';  // renderer.js línea 3
```

**Flujo de operación del Totem:**

```mermaid
sequenceDiagram
    participant T as Totem (Electron)
    participant API as Go API Server (:8080)
    participant S as Sensor (C++/CGO)
    participant DB as SQLite

    loop Cada 500ms (polling)
        T->>API: GET /api/verify_finger
        API->>S: CapturarHuella()
        alt Huella capturada
            S-->>API: plantilla (bytes)
            API->>DB: ObtenerTodosTemplates()
            DB-->>API: map[run]template
            API->>S: CompararHuellas(plantilla, template)
            alt Match >= 300
                API->>DB: ObtenerPerfilPorRunID(run)
                API->>DB: AddRecord(registro)
                API->>API: ticket.EmitirTicket()
                API-->>T: {"type":"ticket","status":"approved","data":{...}}
                T->>T: Pantalla verde ✓
            else No match
                API-->>T: {"type":"no_match","status":"rejected"}
                T->>T: Pantalla roja ✗
            end
        else Sin dedo / timeout
            S-->>API: error
            API-->>T: {"type":"no_match","status":"waiting"}
            T->>T: Sigue en pantalla "waiting"
        end
    end
```

El Totem solo usa **1 endpoint**: `/api/verify_finger`. Todo el flujo de verificación, registro en DB e impresión de ticket se ejecuta del lado del servidor Go.

**¿Cómo se lanza el Totem?**
Desde el menú CLI (opción 4), [ElectronTotem.go](file:///c:/Proyectos/Pydigitador/infra/ElectronTotem.go) ejecuta:
```go
cmd := exec.Command("npx", "electron", "./infra/totem")
```
Esto abre Electron que carga `index.html` del totem.

> [!NOTE]
> La ruta `"./infra/totem"` en `ElectronTotem.go` tiene el **mismo problema** que tenía la DB: depende del directorio de trabajo actual. Debería ajustarse dinámicamente como se hizo con `dbPath`.

---

## 2. Capa de Base de Datos

### Flujo de datos

```mermaid
graph LR
    subgraph "infra/DB"
        REPO["SQLiteUserRepository<br/>(dbRepository.go)"]
    end

    subgraph "core/db"
        MODELS["Models.go<br/>(structs + enums)"]
        SQLITE["digitador.db"]
    end

    REPO -->|"sql.Open(sqlite3)"| SQLITE
    REPO -->|"usa tipos de"| MODELS

    subgraph "Tablas SQLite"
        T1["Usuarios<br/>(run_id, dv, nombre_completo, id_rol, template_huella)"]
        T2["RegistrosRaciones<br/>(id_registro, id_estudiante, fecha_servicio,<br/>tipo_racion, id_terminal, hora_evento, estado_registro)"]
    end

    SQLITE --- T1
    SQLITE --- T2
    T2 -->|"FK: id_estudiante"| T1
```

### Funciones del repositorio ([dbRepository.go](file:///c:/Proyectos/Pydigitador/infra/DB/dbRepository.go))

| Función | Descripción | Usado por |
|---|---|---|
| `NewSQLiteUserRepository(path)` | Abre la conexión SQLite | `infra/main.go` |
| `SaveUser(user)` | INSERT OR REPLACE en Usuarios | Enrolamiento |
| `AddStudent(user)` | Wrapper de SaveUser | API POST |
| `UpdateStudent(user)` | Wrapper de SaveUser | API PUT |
| `DeleteStudentByRun(run)` | DELETE por RUN | API DELETE |
| `GetUser(runID)` | Perfil completo de 1 usuario | Lógica de verificación |
| `GetAllUsers()` | Todos los usuarios | API GET, Dashboard |
| `ObtenerTodosTemplates()` | Map de run→huella | Verificación de identidad |
| `GuardarRegistroRacion(reg)` | Inserta ticket en DB | Flujo de ticket |
| `AddRecord(record)` | Wrapper de GuardarRegistroRacion | API verify_finger |
| `GetRecentRecords(limit)` | JOIN Usuarios+Raciones (recientes) | Dashboard "Registros Recientes" |
| `GetStats()` | Conteo global desayunos/almuerzos | Dashboard stats |
| `GetStudentStats(run)` | Stats por alumno | Modal de estadísticas |
| `GetStudentHistory(run)` | Historial de raciones de 1 alumno | Modal detalle |
| `ObtenerPerfilPorRunID(run)` | → `PerfilEstudiante` | Verificación en Totem |
| `BorrarRegistro()` | Borra último registro (LIFO) | Menú CLI |
| `BorrarUsuario()` | Borra último usuario (LIFO) | Menú CLI |
| `BorrarTodo()` | Limpia toda la DB | Menú CLI (opción 10) |
| `SincronizarRegistros()` | Marca pendientes como sincronizados | API sync |
| `Close()` | Cierra conexión | defer en main |

---

## 3. Mapa Completo de Conexiones del Sistema

```mermaid
graph TB
    subgraph "👤 Usuario"
        CLI["Consola CLI<br/>(menú terminal)"]
        BROWSER["Navegador Web<br/>(Dashboard)"]
        TOTEM_UI["Pantalla Totem<br/>(Electron)"]
        FINGER["Dedo en sensor USB"]
    end

    subgraph "🔵 Entry Point"
        CMD_MAIN["cmd/main.go → infra.Main()"]
    end

    subgraph "🟢 Servidor HTTP (:8080)"
        API_SERVER["ApiRestServer.go"]
        STATIC["FileServer(webpage/)"]
    end

    subgraph "🟡 Lógica de Aplicación"
        APP_ENROLL["enrollUsers.go"]
        APP_VERIFY["VerificarUser.go"]
        APP_TICKET["menuTicket.go"]
        APP_RECENT["menuShowRecent.go"]
        PRINT["ticket.go (impresión)"]
    end

    subgraph "🔴 Infraestructura"
        DB_REPO["dbRepository.go"]
        ELECTRON_LAUNCH["ElectronTotem.go"]
        SENSOR_ADAPT["SensorAdapter.go (CGO)"]
    end

    subgraph "⚫ Core / Hardware"
        SENSOR_CPP_2["Sensor.cpp + SensorBridge.cpp<br/>(SDK DigitalPersona)"]
        SQLITE_DB["digitador.db (SQLite)"]
        USB["Sensor USB físico"]
    end

    CLI --> CMD_MAIN
    CMD_MAIN -->|"opción 1"| APP_ENROLL
    CMD_MAIN -->|"opción 2"| APP_VERIFY
    CMD_MAIN -->|"opción 3"| APP_TICKET
    CMD_MAIN -->|"opción 4"| API_SERVER
    CMD_MAIN -->|"opción 4"| ELECTRON_LAUNCH
    CMD_MAIN -->|"opción 5-6"| APP_RECENT

    BROWSER -->|"HTTP GET/POST"| API_SERVER
    API_SERVER --> STATIC
    STATIC -->|"index.html + app.js"| BROWSER

    TOTEM_UI -->|"HTTP polling /api/verify_finger"| API_SERVER
    ELECTRON_LAUNCH -->|"npx electron"| TOTEM_UI

    API_SERVER --> DB_REPO
    API_SERVER --> SENSOR_ADAPT
    API_SERVER --> PRINT

    APP_ENROLL --> SENSOR_ADAPT
    APP_ENROLL --> DB_REPO
    APP_VERIFY --> SENSOR_ADAPT
    APP_VERIFY --> DB_REPO
    APP_TICKET --> SENSOR_ADAPT
    APP_TICKET --> DB_REPO
    APP_TICKET --> PRINT

    DB_REPO -->|"SQL queries"| SQLITE_DB
    SENSOR_ADAPT -->|"CGO"| SENSOR_CPP_2
    SENSOR_CPP_2 -->|"USB HID"| USB
    FINGER --> USB
```

---

## Resumen de Observaciones

### ✅ Lo que funciona bien
- Arquitectura hexagonal clara: `core/` (dominio) → `app/` (lógica) → `infra/` (infra)
- El servidor API unifica el acceso a datos para ambos frontends
- El repositorio SQLite centraliza todas las operaciones de BD
- El sensor C++ se integra limpiamente via CGO

### ⚠️ Problemas y riesgos detectados

| # | Problema | Impacto | Archivos |
|---|---|---|---|
| 1 | **Ruta del sensor/totem hardcodeada** (`"./infra/totem"`) | Mismo problema que la DB: falla si el exe se ejecuta desde otra carpeta | `ElectronTotem.go` |
| 2 | **Case mismatch en endpoint sensor:** JS llama `/api/sensor/status`, Go registra `/api/Sensor/status` | Puede fallar silenciosamente | `app.js` ↔ `ApiRestServer.go` |
| 3 | **Endpoints de exportación no implementados** (`/api/export/records`, `/api/export/students`) | Botón "Exportar a Excel" no funciona | `app.js` L607, L637 |
| 4 | **Ruta de `webpage/` también es relativa** en `FileServer(http.Dir("webpage"))` | Dashboard no se serviría si ejecutas desde otra carpeta | `ApiRestServer.go` L207 |
| 5 | **Comentarios legacy mencionan C++** en el totem (`main.js` dice "servidor C++") | Confusión, el servidor ahora es Go | `main.js` |
