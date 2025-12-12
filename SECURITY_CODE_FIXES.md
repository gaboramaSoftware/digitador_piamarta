# Digitador Security - Code Examples for Fixes

This document provides copy-paste ready code to fix the security vulnerabilities.

---

## Fix #1: Input Validation (Add to CrowServer.cpp)

### Add regex validation to student enrollment endpoint

**Location:** Before the enrollment logic, around line 218

```cpp
// Add this helper function at the top of CrowServer.cpp
#include <regex>

bool validate_run(const std::string &run) {
    // Chilean RUN format: 7-8 digits + check digit (0-9 or K)
    std::regex pattern("^[0-9]{7,8}[0-9Kk]$");
    return std::regex_match(run, pattern);
}

bool validate_name(const std::string &name) {
    // Only letters, spaces, hyphens, apostrophes
    // Max 100 characters
    if (name.empty() || name.length() > 100) return false;
    std::regex pattern("^[a-zA-Zá-ÿ\\s'-]+$");
    return std::regex_match(name, pattern);
}

bool validate_course(const std::string &curso) {
    // Whitelist valid courses
    static const std::vector<std::string> valid_courses = {
        "1° Básico", "2° Básico", "3° Básico", "4° Básico",
        "5° Básico", "6° Básico", "7° Básico", "8° Básico",
        "1° Medio", "2° Medio", "3° Medio", "4° Medio"
    };
    return std::find(valid_courses.begin(), valid_courses.end(), curso) != valid_courses.end();
}

// Then modify the POST endpoint:
CROW_ROUTE(app, "/api/students")
    .methods("POST"_method)([&db](const crow::request &req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        std::string run = body["run"].s();
        std::string nombre = body["nombre"].s();
        std::string apellido = body["apellido"].s();
        std::string curso = body["curso"].s();

        // ========== VALIDATION ==========
        
        // Validate RUN
        if (!validate_run(run)) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "RUN inválido. Formato: 12345678K";
            return crow::response(400, error);
        }

        // Validate names
        if (!validate_name(nombre)) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Nombre contiene caracteres inválidos";
            return crow::response(400, error);
        }

        if (!validate_name(apellido)) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Apellido contiene caracteres inválidos";
            return crow::response(400, error);
        }

        // Validate course
        if (!validate_course(curso)) {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Curso inválido";
            return crow::response(400, error);
        }

        // Continue with existing logic...
        // (rest of the enrollment code)
    });
```

---

## Fix #2: Fix CSV Injection (Replace in CrowServer.cpp)

**Location:** Lines 326-352

```cpp
// Add this helper function
std::string escape_csv_field(const std::string &field) {
    std::string result = field;
    
    // Check if field needs quoting (contains comma, quote, or newline)
    bool needs_quoting = (field.find(',') != std::string::npos ||
                         field.find('"') != std::string::npos ||
                         field.find('\n') != std::string::npos ||
                         field.find('\r') != std::string::npos ||
                         field.find('=') == 0 ||  // Formula injection
                         field.find('+') == 0 ||
                         field.find('-') == 0 ||
                         field.find('@') == 0);
    
    if (needs_quoting) {
        // Escape internal quotes by doubling them
        size_t pos = 0;
        while ((pos = result.find('"', pos)) != std::string::npos) {
            result.insert(pos, "\"");
            pos += 2;
        }
        // Wrap in quotes
        result = "\"" + result + "\"";
    }
    
    return result;
}

// Then replace the export endpoint:
CROW_ROUTE(app, "/api/export/students")
([&db]() {
    auto students = GetAllStudents(db);

    std::string csv = "RUN,Nombre,Curso\n";
    for (const auto &s : students) {
        csv += escape_csv_field(s.run_id) + "," + 
               escape_csv_field(s.nombre_completo) + "," + 
               escape_csv_field(s.curso) + "\n";
    }

    crow::response res(csv);
    res.add_header("Content-Type", "text/csv; charset=utf-8");
    res.add_header("Content-Disposition", 
                   "attachment; filename=\"estudiantes_" + 
                   std::to_string(std::time(nullptr)) + ".csv\"");
    res.add_header("X-Content-Type-Options", "nosniff");
    return res;
});

// Same for records export:
CROW_ROUTE(app, "/api/export/records")
([&db]() {
    auto records = db.Obtener_Ultimos_Registros(1000);

    std::string csv = "ID,RUN,Fecha,Tipo Ración,Terminal,Estado\n";
    for (const auto &r : records) {
        std::string tipo = (r.tipo_racion == TipoRacion::Desayuno) ? "Desayuno" : "Almuerzo";
        std::string estado = (r.estado_registro == EstadoRegistro::SINCRONIZADO)
                                ? "SINCRONIZADO"
                                : "PENDIENTE";

        csv += escape_csv_field(std::to_string(r.id_registro)) + "," +
               escape_csv_field(r.id_estudiante) + "," +
               escape_csv_field(r.fecha_servicio) + "," +
               escape_csv_field(tipo) + "," +
               escape_csv_field(r.id_terminal) + "," +
               escape_csv_field(estado) + "\n";
    }

    crow::response res(csv);
    res.add_header("Content-Type", "text/csv; charset=utf-8");
    res.add_header("Content-Disposition", 
                   "attachment; filename=\"registros_" + 
                   std::to_string(std::time(nullptr)) + ".csv\"");
    res.add_header("X-Content-Type-Options", "nosniff");
    return res;
});
```

---

## Fix #3: Remove Sensitive Data from Logs

**File:** `src/cpp/CrowServer.cpp`, line 192

**BEFORE:**
```cpp
std::cout << "[CrowServer] Iniciando enrolamiento de: " << nombre
          << " (RUN: " << run << ")" << std::endl;
```

**AFTER:**
```cpp
// Safe logging - no PII
std::cout << "[CrowServer] Starting student enrollment process" << std::endl;

// If you need details for debugging, log to audit system only
// audit_log.Log("ENROLLMENT_START", run, "", "", ""); // No names!
```

**File:** Remove all student name/RUN logging from `src/cpp/DB_Backend.cpp`

Find and replace:
```cpp
// Remove this kind of logging:
std::cerr << "[DB_Backend] ERROR: " << nombre << " already exists\n";

// Replace with:
std::cerr << "[DB_Backend] ERROR: Student enrollment failed (duplicate)\n";
```

---

## Fix #4: Rate Limiting Implementation

**File:** Create new file `src/cpp/RateLimiter.hpp`

```cpp
#pragma once

#include <map>
#include <string>
#include <chrono>
#include <mutex>

class RateLimiter {
private:
    struct RequestInfo {
        int count = 0;
        std::chrono::system_clock::time_point window_start;
    };
    
    std::map<std::string, RequestInfo> requests;
    std::mutex mutex_;
    const int max_requests_per_minute = 100;
    const int max_requests_sensitive = 10;  // For sensitive endpoints like delete

public:
    bool is_allowed(const std::string &ip_address, bool is_sensitive = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto limit = is_sensitive ? max_requests_sensitive : max_requests_per_minute;
        
        auto it = requests.find(ip_address);
        
        // New IP or window expired (>60 seconds)
        if (it == requests.end() || 
            std::chrono::duration_cast<std::chrono::seconds>(now - it->second.window_start) > std::chrono::minutes(1)) {
            RequestInfo info;
            info.count = 1;
            info.window_start = now;
            requests[ip_address] = info;
            return true;
        }
        
        // Within window - check if limit exceeded
        if (it->second.count < limit) {
            it->second.count++;
            return true;
        }
        
        // Limit exceeded
        return false;
    }
    
    void cleanup_old_entries() {
        // Call this periodically to clean up old entries
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        
        for (auto it = requests.begin(); it != requests.end();) {
            if (std::chrono::duration_cast<std::chrono::minutes>(now - it->second.window_start) > std::chrono::minutes(5)) {
                it = requests.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

**File:** Modify `src/cpp/CrowServer.cpp`

```cpp
#include "RateLimiter.hpp"

void runWebServer(DB_Backend &db) {
    crow::SimpleApp app;
    RateLimiter rate_limiter;

    // Cleanup old entries every 5 minutes
    std::thread cleanup_thread([&rate_limiter]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            rate_limiter.cleanup_old_entries();
        }
    });
    cleanup_thread.detach();

    // Now add rate limiting to endpoints:
    
    CROW_ROUTE(app, "/api/students").methods("GET"_method)([&db, &rate_limiter](const crow::request &req) {
        std::string client_ip = req.remote_ip_address;
        
        if (!rate_limiter.is_allowed(client_ip)) {
            crow::json::wvalue error;
            error["error"] = "Rate limit exceeded. Max 100 requests per minute.";
            return crow::response(429, error);  // 429 = Too Many Requests
        }
        
        auto students = GetAllStudents(db);
        std::vector<crow::json::wvalue> list;
        for (const auto &s : students) {
            crow::json::wvalue student;
            student["run"] = s.run_id;
            student["nombre"] = s.nombre_completo;
            student["curso"] = s.curso;
            list.push_back(std::move(student));
        }
        crow::json::wvalue result = crow::json::wvalue::list();
        result = std::move(list);
        return result;
    });
    
    // More restrictive limit for DELETE operations
    CROW_ROUTE(app, "/api/students/<string>")
        .methods("DELETE"_method)([&db, &rate_limiter](const crow::request &req, std::string run) {
            std::string client_ip = req.remote_ip_address;
            
            if (!rate_limiter.is_allowed(client_ip, true)) {  // true = sensitive
                crow::json::wvalue error;
                error["error"] = "Rate limit exceeded. Max 10 delete requests per minute.";
                return crow::response(429, error);
            }
            
            // ... rest of delete logic
        });

    // ... rest of server setup
    app.port(18080).multithreaded().run();
}
```

---

## Fix #5: Audit Logging

**File:** Add to `src/h/DB_Backend.hpp`

```cpp
// Add to class DB_Backend
public:
    // Log an audit event
    bool Log_Audit_Event(const std::string &user_id,
                        const std::string &action,
                        const std::string &table_name,
                        const std::string &record_id,
                        const std::string &old_value,
                        const std::string &new_value,
                        const std::string &ip_address);
```

**File:** Add to `src/cpp/DB_Backend.cpp`

```cpp
// Create audit log table in Inicializar_DB()
// Add to sql_schema:
"CREATE TABLE IF NOT EXISTS AuditLog ("
"    id_log INTEGER PRIMARY KEY AUTOINCREMENT,"
"    timestamp TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
"    user_id TEXT,"
"    action TEXT NOT NULL,"
"    table_name TEXT,"
"    record_id TEXT,"
"    old_value TEXT,"
"    new_value TEXT,"
"    ip_address TEXT"
");"

// Add this method:
bool DB_Backend::Log_Audit_Event(const std::string &user_id,
                                const std::string &action,
                                const std::string &table_name,
                                const std::string &record_id,
                                const std::string &old_value,
                                const std::string &new_value,
                                const std::string &ip_address) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    const char *sql = "INSERT INTO AuditLog "
                      "(user_id, action, table_name, record_id, old_value, new_value, ip_address) "
                      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);";
    
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_handle_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, table_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, record_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, old_value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, new_value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, ip_address.c_str(), -1, SQLITE_TRANSIENT);
    
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}
```

**File:** Use in `CrowServer.cpp`

```cpp
CROW_ROUTE(app, "/api/students/<string>")
    .methods("DELETE"_method)([&db](const crow::request &req, std::string run) {
        bool success = db.Eliminar_Usuario(run);

        if (success) {
            // Log the deletion
            db.Log_Audit_Event(
                "admin_user",  // TODO: Replace with authenticated user
                "DELETE",
                "Usuarios",
                run,
                "",  // old value
                "",  // new value (deleted)
                req.remote_ip_address
            );
            
            crow::json::wvalue result;
            result["success"] = true;
            result["message"] = "Student deleted successfully";
            return crow::response(200, result);
        } else {
            crow::json::wvalue error;
            error["success"] = false;
            error["message"] = "Error deleting student";
            return crow::response(500, error);
        }
    });
```

---

## Fix #6: Remove Hardcoded Admin Credentials

**File:** `src/h/Root_Admin.hpp`

**BEFORE:**
```cpp
namespace Root_Admin {
const std::string DEFAULT_RUN = "11111111";
const std::string DEFAULT_DV = "1";
const std::string DEFAULT_NAME = "Root Admin";
const int DEFAULT_ROLE = 1;
const std::string DEFAULT_PASS_HASH = 
    "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9";
}
```

**AFTER:**
```cpp
namespace Root_Admin {
// DO NOT HARDCODE CREDENTIALS!
// Generate admin user on first run through setup wizard
// Store in encrypted config file, not source code

// Only these constants for system roles
const int ADMIN_ROLE = 1;
const int OPERATOR_ROLE = 2;
const int STUDENT_ROLE = 3;
}
```

**File:** Modify database initialization in `src/cpp/DB_Backend.cpp`

**BEFORE:**
```cpp
const char *sql_admin =
    "INSERT OR IGNORE INTO Usuarios "
    "(run_id, dv, nombre_completo, id_rol, password_hash, template_huella) "
    "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
    
sqlite3_bind_text(stmt_admin, 1, Root_Admin::DEFAULT_RUN.c_str(), -1, SQLITE_STATIC);
sqlite3_bind_text(stmt_admin, 2, Root_Admin::DEFAULT_DV.c_str(), -1, SQLITE_STATIC);
sqlite3_bind_text(stmt_admin, 3, Root_Admin::DEFAULT_NAME.c_str(), -1, SQLITE_STATIC);
```

**AFTER:**
```cpp
// Don't auto-create admin user in database
// Instead, check if ANY admin exists, if not, require setup wizard
const char *sql_check_admin =
    "SELECT COUNT(*) FROM Usuarios WHERE id_rol = 1;";

sqlite3_stmt *stmt_check = nullptr;
if (sqlite3_prepare_v2(db_handle_, sql_check_admin, -1, &stmt_check, nullptr) == SQLITE_OK) {
    if (sqlite3_step(stmt_check) == SQLITE_ROW) {
        int admin_count = sqlite3_column_int(stmt_check, 0);
        if (admin_count == 0) {
            std::cerr << "[DB_Backend] WARNING: No admin user found!\n"
                      << "Please run setup wizard to create admin account.\n";
            // Don't auto-create - force explicit setup
        }
    }
    sqlite3_finalize(stmt_check);
}
```

---

## Fix #7: Security Headers

**File:** Add helper function to `src/cpp/CrowServer.cpp`

```cpp
void add_security_headers(crow::response &res) {
    // Prevent MIME type sniffing
    res.add_header("X-Content-Type-Options", "nosniff");
    
    // Prevent clickjacking
    res.add_header("X-Frame-Options", "DENY");
    
    // HTTPS enforcement
    res.add_header("Strict-Transport-Security", 
                   "max-age=31536000; includeSubDomains");
    
    // Content Security Policy
    res.add_header("Content-Security-Policy", 
                   "default-src 'self'; "
                   "script-src 'self'; "
                   "style-src 'self' https://fonts.googleapis.com; "
                   "font-src https://fonts.gstatic.com; "
                   "img-src 'self' data:; "
                   "connect-src 'self'");
    
    // XSS Protection
    res.add_header("X-XSS-Protection", "1; mode=block");
    
    // Disable referrer
    res.add_header("Referrer-Policy", "strict-origin-when-cross-origin");
}

// Use in every route:
CROW_ROUTE(app, "/api/students").methods("GET"_method)([&db](const crow::request &req) {
    auto students = GetAllStudents(db);
    // ... build response ...
    crow::response res;
    // ... set body ...
    add_security_headers(res);
    return res;
});
```

---

## Fix #8: Backup Script

**File:** Create `backup_database.bat`

```batch
@echo off
setlocal enabledelayedexpansion

REM Digitador Database Backup Script
REM Run via Windows Task Scheduler daily at 2 AM

echo [%date% %time%] Starting backup...

REM Create backup directory if it doesn't exist
if not exist "BD\backups" mkdir "BD\backups"

REM Generate timestamp
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set yy=%%c& set mm=%%a& set dd=%%b)
for /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set hh=%%a& set min=%%b)

REM Create backup with timestamp
set backup_file=BD\backups\digitador_!yy!!mm!!dd!_!hh!!min!.db.backup

copy "BD\digitador.db" "!backup_file!" >nul 2>&1

if %errorlevel% == 0 (
    echo [%date% %time%] Backup completed: !backup_file!
    
    REM Keep only last 30 days of backups
    REM (Optional: implement cleanup logic here)
    
) else (
    echo [%date% %time%] ERROR: Backup failed!
    exit /b 1
)

endlocal
```

**To schedule in Windows Task Scheduler:**
1. Open Task Scheduler
2. Create Basic Task
3. Name: "Digitador Daily Backup"
4. Trigger: Daily at 2:00 AM
5. Action: Start a program → Select backup_database.bat

---

## Fix #9: Security Testing Script

**File:** Create `test_security.ps1`

```powershell
# Digitador Security Testing Script

$BASE_URL = "http://localhost:18080"
$CRITICAL = 0
$WARNINGS = 0

function Test-SQL-Injection {
    Write-Host "Testing SQL Injection..." -ForegroundColor Yellow
    
    $payloads = @(
        "'1'='1",
        "'; DROP TABLE Usuarios; --",
        "1 UNION SELECT * FROM Usuarios"
    )
    
    foreach ($payload in $payloads) {
        $response = curl.exe -s "$BASE_URL/api/students" `
            -H "Content-Type: application/json" `
            -d "{`"run`":`"$payload`"}" | ConvertFrom-Json
        
        if ($response.error -match "syntax error|SQL") {
            Write-Host "  ❌ VULNERABLE to SQL Injection: $payload" -ForegroundColor Red
            $CRITICAL += 1
        }
    }
}

function Test-Authentication {
    Write-Host "Testing Authentication..." -ForegroundColor Yellow
    
    $response = curl.exe -s "$BASE_URL/api/students/20468300K/stats"
    if ($response -notmatch "401|Unauthorized|token") {
        Write-Host "  ❌ VULNERABLE: No authentication required" -ForegroundColor Red
        $CRITICAL += 1
    }
}

function Test-Rate-Limiting {
    Write-Host "Testing Rate Limiting..." -ForegroundColor Yellow
    
    for ($i = 0; $i -lt 150; $i++) {
        $response = curl.exe -s -o /dev/null -w "%{http_code}" `
            "$BASE_URL/api/students"
        
        if ($response -eq "429") {
            Write-Host "  ✓ Rate limiting active after $i requests" -ForegroundColor Green
            return
        }
    }
    
    Write-Host "  ❌ No rate limiting detected (150+ requests allowed)" -ForegroundColor Red
    $WARNINGS += 1
}

function Test-HTTPS {
    Write-Host "Testing HTTPS..." -ForegroundColor Yellow
    
    $response = curl.exe -s -o /dev/null -w "%{http_code}" `
        "https://localhost:18080/api/students" 2>&1
    
    if ($response -eq "200" -or $response -eq "401") {
        Write-Host "  ✓ HTTPS is configured" -ForegroundColor Green
    } else {
        Write-Host "  ❌ HTTPS not working or not configured" -ForegroundColor Red
        $CRITICAL += 1
    }
}

function Test-Input-Validation {
    Write-Host "Testing Input Validation..." -ForegroundColor Yellow
    
    # Test XSS payload
    $xss_payload = "<script>alert('xss')</script>"
    $response = curl.exe -s "$BASE_URL/api/students" `
        -H "Content-Type: application/json" `
        -d "{`"run`":`"$xss_payload`"}" 2>&1
    
    if ($response -match "Invalid|error" -and $response -notmatch "script") {
        Write-Host "  ✓ Input validation detected (rejected XSS)" -ForegroundColor Green
    } else {
        Write-Host "  ❌ Input validation not working" -ForegroundColor Red
        $WARNINGS += 1
    }
}

# Run all tests
Write-Host "===============================" -ForegroundColor Cyan
Write-Host "Digitador Security Test Suite" -ForegroundColor Cyan
Write-Host "===============================" -ForegroundColor Cyan
Write-Host ""

Test-Authentication
Test-HTTPS
Test-Rate-Limiting
Test-Input-Validation
Test-SQL-Injection

Write-Host ""
Write-Host "===============================" -ForegroundColor Cyan
Write-Host "Results: $CRITICAL Critical | $WARNINGS Warnings" -ForegroundColor Cyan
Write-Host "===============================" -ForegroundColor Cyan

if ($CRITICAL -gt 0) {
    exit 1
} else {
    exit 0
}
```

Run with:
```powershell
powershell -ExecutionPolicy Bypass -File test_security.ps1
```

---

## Summary of Changes

| Fix | File | Complexity | Time |
|-----|------|-----------|------|
| Input Validation | CrowServer.cpp | Medium | 2 hrs |
| CSV Injection | CrowServer.cpp | Low | 1 hr |
| Remove Logs | Multiple | Low | 1 hr |
| Rate Limiting | New file | Medium | 2 hrs |
| Audit Logging | DB_Backend | Medium | 3 hrs |
| Remove Creds | Root_Admin.hpp | Low | 30 min |
| Security Headers | CrowServer.cpp | Low | 1 hr |
| Backup Script | New file | Low | 1 hr |

**Total: ~12-13 hours of work**

Start with the highest impact fixes first!

