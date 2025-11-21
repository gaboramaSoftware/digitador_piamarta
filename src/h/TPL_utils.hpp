#pragma once

//includes
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

inline void guardarImagenComoBPM(const std::vector<unsigned char>& imagenCruda, int ancho, int alto, const std::string& filename){
    if(imagenCruda.empty()){
        std::cerr << "[Imagen] Error: el buffer de imagen esta vacio." << std::endl;
        return;
    }

    if (!file){
        std::cerr << "[Imagen] Error: No se puede crear el archivo" <<filename << std::endl;
        return;
    }

    //Calcular el pading de los archivos brutos
    int paddingSize = (4 - (width % 4)) % 4;

    int fileHeaderSize = 14;
    int infoHeaderSize = 40;
    int colorTableSize = 256 *4; //tabla de colores

    //tamaño total del archivo
    int fileSize = fileHeaderSize + infoHeaderSize + colorTableSize + (width + paddingSize) * height;


    // bpm file header
    unsigned char fileHeader[14] = {
        'B', 'M',
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
    };

    //llenar espacio de los archivos
    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);

    //LLenar el offset
    int pixelDataOffset = fileHeaderSize + infoHeaderSize + colorTableSize;
    fileHeader[10] = (unsigned char)(pixelDataOffset);
    fileHeader[11] = (unsigned char)(pixelDataOffset >> 8);
    fileHeader[12] = (unsigned char)(pixelDataOffset >> 16);
    fileHeader[13] = (unsigned char)(pixelDataOffset >> 24);

    file.write(reinterpret_cast<char*>(fileHeader), 14);

    // --- 2. BPM Info Header (40 bytes) ---
    unsigned char infoHeader[40] = {
        40, 0, 0, 0, //tamaño del header
        0, 0, 0, 0, //ancho del header
        0, 0, 0, 0, //altura del header
        1, 0, //plano (siempre deberia ser 1)
        8, 0, //bits por pixel (8 en greyscale)
        0, 0, 0, 0, //compresion de imagen (0 = null)
        0, 0, 0, 0, //tamaño de imagen 
        0, 0, 0, 0, //X pixeles * meter
        0, 0, 0, 0, //Y pixeles * meter
        0, 0, 0, 0, //Calidad de color (0 = max)
        0, 0, 0, 0, //Colores importantes (grises) 
    };

    //LLenar por ancho
    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);

    // Llenar por alto
    infoHeader[8] = (unsigned char)(height);
    infoHeader[9] = (unsigned char)(height >> 8);
    infoHeader[10] = (unsigned char)(height >> 16);
    infoHeader[11] = (unsigned char)(height >> 24);

    file.write(reinterpret_cast<char*>(infoHeader), 40);

    for (int i = 0; i < 256; i++){
        unsigned char color[4] = { 
            (unsignedchar)i,
            (unsigned char)i,
            (unsigned char)i,
            0 };
        
        filewrite(reinterpret_cast<char*>(color), 4);
    }

    // guardar los pixeles (bottom up)
    unsigned char padding[3] = { 0, 0, 0 };
    for (int y = height - 1; y >= 0; y--){
        file.write(reinterpret_cast<const char*>(&rawImageBuffer[y * width]), width);

        //Escribe padding si es necesario para alinear a 4 bytes
        if(paddingSize > 0) file.write(reinterpret_cast<char*>(padding), paddingSize);
    }

    file.close();
    std::cout << "[Imagen]: Imagen guardada correctamente"; << filename << std::endl;

}