#pragma once
#include <algorithm>
#include <iostream> // Para debug
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// Incluimos la librería pequeña de C
#include "aes.h"

class CryptoUtils {
private:
  // =========================================================
  //  LLAVE MAESTRA (256-bit / 32 bytes)
  // =========================================================
  inline static const uint8_t MASTER_KEY[32] = {
      0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33,
      0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee,
      0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

public:
  // --- ENCRIPTAR ---
  // Recibe: Vector de bytes crudo (Template ZKTeco)
  // Retorna: Vector con [ IV (16 bytes) | CIFRADO ... ]
  static std::vector<uint8_t>
  EncriptarTemplate(const std::vector<uint8_t> &datos_crudos) {
    if (datos_crudos.empty())
      return {};

    // 1. Generar un IV (Initialization Vector) aleatorio de 16 bytes
    uint8_t iv[AES_BLOCKLEN];
    generarIVAleatorio(iv);

    // 2. Preparar buffer de salida: Reservamos espacio
    std::vector<uint8_t> salida;
    salida.reserve(AES_BLOCKLEN + datos_crudos.size());

    // 3. Copiamos el IV al principio del vector de salida
    salida.insert(salida.end(), iv, iv + AES_BLOCKLEN);

    // 4. Copiamos los datos crudos después del IV
    salida.insert(salida.end(), datos_crudos.begin(), datos_crudos.end());

    // 5. Inicializar Contexto AES
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, MASTER_KEY, iv);

    // 6. Encriptar IN-PLACE (desde el byte 16 en adelante)
    // OJO: Apuntamos a salida.data() + 16
    AES_CTR_xcrypt_buffer(&ctx, salida.data() + AES_BLOCKLEN,
                          datos_crudos.size());

    return salida;
  }

  // --- DESENCRIPTAR ---
  // Recibe: Vector encriptado [ IV (16) | CIFRADO ... ]
  // Retorna: Vector desencriptado original
  static std::vector<uint8_t>
  DesencriptarTemplate(const std::vector<uint8_t> &datos_encriptados) {
    // Validación mínima
    if (datos_encriptados.size() <= AES_BLOCKLEN) {
      std::cerr << "[Crypto] Error: Dato muy corto para desencriptar.\n";
      return {};
    }

    // 1. Extraer el IV (los primeros 16 bytes)
    uint8_t iv[AES_BLOCKLEN];
    std::copy(datos_encriptados.begin(),
              datos_encriptados.begin() + AES_BLOCKLEN, iv);

    // 2. Extraer el Ciphertext (el resto)
    std::vector<uint8_t> salida(datos_encriptados.begin() + AES_BLOCKLEN,
                                datos_encriptados.end());

    // 3. Inicializar Contexto AES
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, MASTER_KEY, iv);

    // 4. Desencriptar (CTR usa la misma función que encriptar)
    AES_CTR_xcrypt_buffer(&ctx, salida.data(), salida.size());

    return salida;
  }

private:
  static void generarIVAleatorio(uint8_t *buffer) {
    // Generador de números aleatorios moderno de C++
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < AES_BLOCKLEN; ++i) {
      buffer[i] = static_cast<uint8_t>(dis(gen));
    }
  }
};