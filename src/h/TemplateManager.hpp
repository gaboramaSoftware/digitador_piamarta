// templateManager.hpp

#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


#include "DB_Backend.hpp"
#include "DB_models.hpp"
#include "sensor.h"


namespace fs = std::filesystem;

// funciones de logica de menu
void menuEnroll(Sensor &sensor, DB_Backend &db);
void menuVerify(Sensor &sensor, DB_Backend &db);
void menuProcessTicket(Sensor &sensor, DB_Backend &db);
void menuShowRecent(DB_Backend &db);

// Guarda una imagen en escala de grises (8 bits) como BMP.
// - imagenCruda: buffer de pixels (un byte por pixel, 0–255).
// - ancho, alto: dimensiones de la imagen.
// - filename: nombre del archivo .bmp a crear.
inline void guardarImagenComoBMP(const std::vector<unsigned char> &imagenCruda,
                                 int ancho, int alto,
                                 const std::string &filename) {
  if (imagenCruda.empty()) {
    std::cerr << "[Imagen] Error: el buffer de imagen está vacío." << std::endl;
    return;
  }

  if (ancho <= 0 || alto <= 0) {
    std::cerr << "[Imagen] Error: dimensiones inválidas (" << ancho << "x"
              << alto << ")." << std::endl;
    return;
  }

  const int width = ancho;
  const int height = alto;

  // BMP de 8 bits: 1 byte por pixel, cada fila se alinea a múltiplo de 4 bytes.
  int rowSize = ((width + 3) / 4) * 4;
  int paddingSize = rowSize - width;
  int pixelDataSize = rowSize * height;
  int paletteSize = 256 * 4;              // 256 colores * (B,G,R,0)
  int dataOffset = 14 + 40 + paletteSize; // headers + paleta
  int fileSize = dataOffset + pixelDataSize;

  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "[Imagen] Error: no se pudo crear el archivo '" << filename
              << "'." << std::endl;
    return;
  }

  // --- 1. File Header (14 bytes) ---
  unsigned char fileHeader[14] = {0};
  fileHeader[0] = 'B';
  fileHeader[1] = 'M';

  fileHeader[2] = (unsigned char)(fileSize & 0xFF);
  fileHeader[3] = (unsigned char)((fileSize >> 8) & 0xFF);
  fileHeader[4] = (unsigned char)((fileSize >> 16) & 0xFF);
  fileHeader[5] = (unsigned char)((fileSize >> 24) & 0xFF);

  // Reservado [6..9] se queda en 0
  fileHeader[10] = (unsigned char)(dataOffset & 0xFF);
  fileHeader[11] = (unsigned char)((dataOffset >> 8) & 0xFF);
  fileHeader[12] = (unsigned char)((dataOffset >> 16) & 0xFF);
  fileHeader[13] = (unsigned char)((dataOffset >> 24) & 0xFF);

  file.write(reinterpret_cast<char *>(fileHeader), sizeof(fileHeader));

  // --- 2. Info Header (40 bytes) ---
  unsigned char infoHeader[40] = {0};
  infoHeader[0] = 40; // tamaño del header

  // Ancho
  infoHeader[4] = (unsigned char)(width & 0xFF);
  infoHeader[5] = (unsigned char)((width >> 8) & 0xFF);
  infoHeader[6] = (unsigned char)((width >> 16) & 0xFF);
  infoHeader[7] = (unsigned char)((width >> 24) & 0xFF);

  // Alto
  infoHeader[8] = (unsigned char)(height & 0xFF);
  infoHeader[9] = (unsigned char)((height >> 8) & 0xFF);
  infoHeader[10] = (unsigned char)((height >> 16) & 0xFF);
  infoHeader[11] = (unsigned char)((height >> 24) & 0xFF);

  // Planos (siempre 1)
  infoHeader[12] = 1;
  infoHeader[13] = 0;

  // Bits por pixel: 8 (escala de grises)
  infoHeader[14] = 8;
  infoHeader[15] = 0;

  // Compresión = 0 (BI_RGB)
  infoHeader[16] = 0;
  infoHeader[17] = 0;
  infoHeader[18] = 0;
  infoHeader[19] = 0;

  // Tamaño de la imagen (solo la data de pixels)
  infoHeader[20] = (unsigned char)(pixelDataSize & 0xFF);
  infoHeader[21] = (unsigned char)((pixelDataSize >> 8) & 0xFF);
  infoHeader[22] = (unsigned char)((pixelDataSize >> 16) & 0xFF);
  infoHeader[23] = (unsigned char)((pixelDataSize >> 24) & 0xFF);

  // Resolución y demás campos se pueden dejar en 0.

  file.write(reinterpret_cast<char *>(infoHeader), sizeof(infoHeader));

  // --- 3. Paleta de 256 grises (B, G, R, 0) ---
  unsigned char palette[256 * 4];
  for (int i = 0; i < 256; ++i) {
    palette[i * 4 + 0] = static_cast<unsigned char>(i); // B
    palette[i * 4 + 1] = static_cast<unsigned char>(i); // G
    palette[i * 4 + 2] = static_cast<unsigned char>(i); // R
    palette[i * 4 + 3] = 0;                             // reservado
  }
  file.write(reinterpret_cast<char *>(palette), sizeof(palette));

  // --- 4. Datos de la imagen (bottom-up) ---
  std::vector<unsigned char> padding(paddingSize, 0);

  for (int y = height - 1; y >= 0; --y) {
    const unsigned char *rowPtr = &imagenCruda[y * width];
    file.write(reinterpret_cast<const char *>(rowPtr), width);

    if (paddingSize > 0) {
      file.write(reinterpret_cast<const char *>(padding.data()), paddingSize);
    }
  }

  if (!file) {
    std::cerr << "[Imagen] Error: fallo al escribir el archivo '" << filename
              << "'." << std::endl;
  } else {
    std::cout << "[Imagen] Imagen guardada correctamente: " << filename
              << std::endl;
  }
}
