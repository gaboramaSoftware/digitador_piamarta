# Digitador Piamarta - Security Remediation Quick Start Guide

## 🚨 CRITICAL: Read This First

**Your application has 5 CRITICAL vulnerabilities that make it completely unsafe for production.**

### The Big 5 Critical Issues:
1. **NO AUTHENTICATION** - Anyone can access all student data
2. **NO HTTPS** - All data travels in plain text over network
3. **Hardcoded encryption key** - In source code (reversible)
4. **Hardcoded admin credentials** - In source code
5. **No password hashing** - Default admin hash is crackable SHA256

**DO NOT DEPLOY** until these are fixed.

---

## Quick Win Fixes (Can do today)

### 1. Remove Hardcoded Credentials (30 minutes)

**File:** `src/h/Root_Admin.hpp`

Replace this:
```cpp
const std::string DEFAULT_RUN = "11111111";
const std::string DEFAULT_DV = "1";
const std::string DEFAULT_NAME = "Root Admin";
const std::string DEFAULT_PASS_HASH = 
    "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9";
```

With this:
```cpp
// Generate these at runtime, not compile time
// Use random UUID for admin RUN
// Don't store in source code!
```

### 2. Add Input Validation (1 hour)

**File:** `src/cpp/CrowServer.cpp`

Add this to the enrollment endpoint (around line 218):

```cpp
// Validate RUN format
std::regex run_pattern("^[0-9]{7,8}[0-9Kk]$");
if (!std::regex_match(run, run_pattern)) {
    return crow::response(400, "Invalid RUN format");
}

// Validate name length
if (nombre.empty() || nombre.length() > 100) {
    return crow::response(400, "Invalid name length (1-100 characters)");
}

// Validate name characters (no numbers, special chars)
std::regex name_pattern("^[a-zA-Zá-ÿ\\s'-]+$");
if (!std::regex_match(nombre, name_pattern)) {
    return crow::response(400, "Name contains invalid characters");
}
```

### 3. Remove Sensitive Data from Logs (30 minutes)

**File:** `src/cpp/CrowServer.cpp`, line 192

Change:
```cpp
std::cout << "[CrowServer] Iniciando enrolamiento de: " << nombre
          << " (RUN: " << run << ")" << std::endl;
```

To:
```cpp
std::cout << "[CrowServer] Iniciando enrolamiento de nuevo estudiante" << std::endl;
// Log to secure audit log instead, not stdout
```

**File:** `src/cpp/DB_Backend.cpp` - Remove all PII logging

### 4. Fix CSV Injection (30 minutes)

**File:** `src/cpp/CrowServer.cpp`, lines 326-352

Change:
```cpp
csv += s.run_id + "," + s.nombre_completo + "," + s.curso + "\n";
```

To:
```cpp
// Escape fields for CSV
auto escape_csv = [](const std::string &s) {
    std::string result = "\"" + s + "\"";
    // Replace quotes with double quotes
    size_t pos = 0;
    while ((pos = result.find("\"", pos + 1)) != std::string::npos) {
        result.insert(pos, "\"");
        pos += 2;
    }
    return result;
};

csv += escape_csv(s.run_id) + "," + 
       escape_csv(s.nombre_completo) + "," + 
       escape_csv(s.curso) + "\n";
```

---

## Medium-Effort Fixes (This week)

### 5. Implement Basic Rate Limiting (3-4 hours)

Add this header to `CrowServer.hpp`:
```cpp
#include <map>
#include <chrono>
#include <string>

class RateLimiter {
    std::map<std::string, std::pair<int, std::chrono::time_point<std::chrono::system_clock>>> requests;
    int max_requests = 100;  // per minute
    
public:
    bool is_allowed(const std::string &ip_address) {
        auto now = std::chrono::system_clock::now();
        auto it = requests.find(ip_address);
        
        if (it == requests.end() || 
            std::chrono::duration_cast<std::chrono::minutes>(now - it->second.second) >= std::chrono::minutes(1)) {
            requests[ip_address] = {1, now};
            return true;
        }
        
        if (it->second.first < max_requests) {
            it->second.first++;
            return true;
        }
        
        return false;  // Rate limit exceeded
    }
};
```

### 6. Create Audit Logging Table (2-3 hours)

Add to database schema in `DB_Backend.cpp`:

```cpp
"CREATE TABLE IF NOT EXISTS AuditLog ("
"    id_log INTEGER PRIMARY KEY AUTOINCREMENT,"
"    timestamp TEXT NOT NULL,"
"    user_id TEXT,"
"    action TEXT NOT NULL,"
"    table_name TEXT,"
"    record_id TEXT,"
"    old_value TEXT,"
"    new_value TEXT,"
"    ip_address TEXT"
");"
```

Then add function to log all modifications.

### 7. Database Backup Script (1-2 hours)

Create `backup_database.bat`:

```batch
@echo off
setlocal enabledelayedexpansion

REM Daily backup of SQLite database
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%c-%%a-%%b)
for /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set mytime=%%a-%%b)

set backup_dir=BD\backups
if not exist %backup_dir% mkdir %backup_dir%

REM Copy database with timestamp
copy BD\digitador.db "%backup_dir%\digitador_!mydate!_!mytime!.db"

echo Backup completed: %backup_dir%\digitador_!mydate!_!mytime!.db
```

Run via Task Scheduler every day at 2 AM.

---

## Hard Stuff (Must do before production - 2-3 weeks)

### 8. Implement JWT Authentication (1 week)

You need:
1. JWT library (header-only: nlohmann/json-jwt)
2. Login endpoint
3. Auth middleware for all protected routes

**Simple JWT Implementation Example:**

```cpp
// Add to endpoints
CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
([&db](const crow::request &req) {
    auto body = crow::json::load(req.body);
    
    // Verify credentials against database (bcrypt hashed)
    std::string username = body["username"].s();
    std::string password = body["password"].s();
    
    // TODO: Verify against database
    
    // Generate JWT token
    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(24))
        .set_payload_claim("user_id", jwt::claim(std::string(username)))
        .set_payload_claim("role", jwt::claim(std::string("admin")))
        .sign(jwt::algorithm::hs256{"your_secret_key"});
    
    crow::json::wvalue result;
    result["token"] = token;
    return crow::response(200, result);
});

// Middleware for protected routes
auto verify_token = [](const crow::request& req) -> std::pair<bool, std::string> {
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.empty()) return {false, ""};
    
    // Extract token from "Bearer <token>"
    // TODO: Verify token
    return {true, "user_id"};
};
```

### 9. Move Encryption Key to Configuration (2 hours)

Create `config.json`:
```json
{
    "encryption_key": "load_from_secure_file_or_environment",
    "database_path": "BD/digitador.db",
    "port": 18080,
    "jwt_secret": "load_from_secure_file_or_environment"
}
```

Load at startup:
```cpp
#include <nlohmann/json.hpp>

auto load_config(const std::string &config_path) {
    std::ifstream f(config_path);
    auto config = nlohmann::json::parse(f);
    return config;
}

// In main:
auto config = load_config("config.json");
std::string enc_key = config["encryption_key"];
```

### 10. Implement HTTPS (2-3 hours)

Use OpenSSL with Crow:

```cpp
// In CrowServer.cpp
#include <openssl/ssl.h>

void runWebServer(DB_Backend &db) {
    crow::SimpleApp app;
    
    // ... all your routes ...
    
    // HTTPS setup
    std::string cert_path = "certs/certificate.crt";
    std::string key_path = "certs/private.key";
    
    app.port(18080)
       .ssl_file(cert_path, key_path)
       .multithreaded()
       .run();
}
```

To generate self-signed certificate (for testing):
```bash
openssl req -x509 -newkey rsa:4096 -keyout private.key -out certificate.crt -days 365 -nodes
```

For production, use Let's Encrypt (free).

---

## Security Headers Checklist

Add these response headers to all endpoints:

```cpp
void add_security_headers(crow::response &res) {
    res.add_header("X-Content-Type-Options", "nosniff");
    res.add_header("X-Frame-Options", "DENY");
    res.add_header("Strict-Transport-Security", 
                   "max-age=31536000; includeSubDomains");
    res.add_header("Content-Security-Policy", 
                   "default-src 'self'; script-src 'self'");
    res.add_header("X-XSS-Protection", "1; mode=block");
}

// Use in every route after creating response
```

---

## Testing Checklist

Before deploying, test:

```bash
# 1. Check dependencies for vulnerabilities
npm audit
npm audit fix

# 2. Test input validation
# Try to enroll student with:
#   - Name: "<script>alert('xss')</script>"
#   - RUN: "'; DROP TABLE Usuarios; --"
#   - Name: (paste 10000 character string)

# 3. Test rate limiting
# Rapid fire 150 requests to same endpoint, should get 429 after 100

# 4. Test authentication
# Try to access /api/students without token, should get 401

# 5. Test HTTPS
# curl https://localhost:18080/ (should work with certificate)
# curl http://localhost:18080/ (should redirect to HTTPS)

# 6. Test audit logging
# Delete a student, check audit log shows deletion

# 7. Verify no hardcoded credentials
strings bin/server.exe | grep -i "password\|admin\|key"
# Should return nothing
```

---

## Dependencies to Add

```bash
npm install bcrypt jsonwebtoken dotenv express-rate-limit
# For C++: Use header-only libraries (no compile overhead)
# - nlohmann/json (JSON parsing)
# - jwt (JWT tokens)
# - cxxopts (config parsing)
```

---

## Timeline

| Week | Task | Status |
|------|------|--------|
| Now | Quick fixes (validation, logs, CSV) | ❌ TODO |
| 1 | Input validation + rate limiting | ❌ TODO |
| 2 | JWT authentication + password hashing | ❌ TODO |
| 3 | HTTPS + audit logging | ❌ TODO |
| 4 | Backup system + final testing | ❌ TODO |

---

## If You Only Have 1 Week

Do this (in order):

1. Implement JWT authentication (3 days)
2. Add HTTPS (1 day)
3. Input validation (1 day)
4. Remove hardcoded creds (few hours)

This covers the 5 critical issues. Other stuff can follow in future sprints.

---

## Need Help?

Common security libraries for C++:

- **JWT**: `https://github.com/Thalhammer/jwt-cpp`
- **Crypto**: `OpenSSL` (already in system)
- **JSON**: `nlohmann/json`
- **Password Hashing**: `bcrypt.h` (wrapper around OpenSSL)

---

## Final Checklist Before Going Live

- [ ] All 5 critical vulnerabilities fixed
- [ ] HTTPS enabled and certificate valid
- [ ] Authentication required for all sensitive endpoints
- [ ] Input validation on all user inputs
- [ ] Audit logging for all modifications
- [ ] Database backups running
- [ ] No hardcoded credentials in code
- [ ] No sensitive data in logs
- [ ] Security headers implemented
- [ ] Rate limiting active
- [ ] Dependencies audited for CVEs
- [ ] Penetration testing completed
- [ ] Code review by security-aware developer
- [ ] Legal review for compliance

---

**Remember:** Security is a process, not a product. This is just the beginning!

