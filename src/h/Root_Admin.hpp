// Root_Admin.hpp
// src/h/Root_Admin.hpp
#pragma once
#include <string>

namespace Root_Admin {

// DATOS PÚBLICOS (No importa si alguien los ve en el binario)
const std::string DEFAULT_RUN = "11111111";
const std::string DEFAULT_DV = "1";
const std::string DEFAULT_NAME = "Root Admin";

// ROL ADMIN
const int DEFAULT_ROLE = 1;

// --- SEGURIDAD ---
const std::string DEFAULT_PASS_HASH =
    "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9";

} // namespace Root_Admin
