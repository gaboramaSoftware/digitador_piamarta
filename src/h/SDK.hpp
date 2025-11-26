//h/sdk.hpp

#pragma once

#include <windows.h>   
#include <cstdint>
#include <string>
#include <functional>   // std::hash

// Incluimos el SDK oficial de ZKTeco SOLO aquí
#include "libzkfp.h"

// Pequeño contexto que agrupa el HANDLE del dispositivo y el del "DB cache"
struct fp_device {
    HANDLE hDevice = nullptr;   // Dispositivo físico (lector)
    HANDLE hDB     = nullptr;   // Cache / DB de huellas en RAM del SDK
};

// Abre el dispositivo y prepara la DB interna del SDK
inline fp_device* fp_open(int index) {
    auto* ctx = new fp_device();

    // 1) Abrir el dispositivo físico
    ctx->hDevice = ZKFPM_OpenDevice(index);
    if (!ctx->hDevice) {
        delete ctx;
        return nullptr;
    }

    // 2) Crear/Inicializar el "DB cache" de huellas en RAM
    ctx->hDB = ZKFPM_DBInit();  // En el .h salía como "same as CreateDBCache, for new version"
    if (!ctx->hDB) {
        ZKFPM_CloseDevice(ctx->hDevice);
        delete ctx;
        return nullptr;
    }

    return ctx;
}

// Cerrar todo ordenadamente
inline void fp_close(fp_device* ctx) {
    if (!ctx) return;

    if (ctx->hDB) {
        // DBFree == CloseDBCache para la versión nueva (según comentarios del .h)
        ZKFPM_DBFree(ctx->hDB);
        ctx->hDB = nullptr;
    }

    if (ctx->hDevice) {
        ZKFPM_CloseDevice(ctx->hDevice);
        ctx->hDevice = nullptr;
    }

    delete ctx;
}

// En tu diseño viejo llamabas a fp_db_init(dev_handle) después de abrir.
// Como ya inicializamos la DB dentro de fp_open, aquí solo devolvemos OK/ERROR.
inline int fp_db_init(fp_device* ctx) {
    return (ctx && ctx->hDB) ? 0 : -1;
}

// Agregar una template al cache interno del SDK
// run_id = RUN del estudiante, lo convertimos a un entero estable (fid)
inline int fp_db_add(fp_device* ctx,
                     const char* run_id,
                     const std::uint8_t* tpl,
                     std::size_t len) {
    if (!ctx || !ctx->hDB || !run_id || !tpl) return -1;

    // Mapear RUN -> fid (unsigned int) usando un hash (por ahora)
    std::uint32_t fid = static_cast<std::uint32_t>(
        std::hash<std::string>{}(std::string(run_id))
    );

    return ZKFPM_DBAdd(
        ctx->hDB,
        fid,
        const_cast<unsigned char*>(tpl),
        static_cast<unsigned int>(len)
    );
}

// Capturar huella -> template directamente (ignoramos la imagen cruda)
inline int fp_capture(fp_device* ctx,
                      std::uint8_t* out_tpl,
                      unsigned int* tpl_size) {
    if (!ctx || !ctx->hDevice || !out_tpl || !tpl_size) return -1;

    // No pedimos imagen (pasamos nullptr y tamaño 0)
    return ZKFPM_AcquireFingerprint(
        ctx->hDevice,
        nullptr,          // fpImage
        0,                // cbFPImage
        reinterpret_cast<unsigned char*>(out_tpl),
        tpl_size
    );
}

// Buscar una huella en el cache del SDK
// Por ahora NO podemos reconstruir el RUN original desde el fid (hash invertido),
// así que devolvemos rc y opcionalmente el score; out_run lo dejamos vacío.
inline int fp_db_identify(fp_device* ctx,
                          const std::uint8_t* tpl,
                          unsigned int tpl_size,
                          std::string& out_run,
                          unsigned int* out_score) {
    if (!ctx || !ctx->hDB || !tpl) return -1;

    unsigned int fid   = 0;
    unsigned int score = 0;

    int rc = ZKFPM_DBIdentify(
        ctx->hDB,
        const_cast<unsigned char*>(tpl),
        tpl_size,
        &fid,
        &score
    );

    if (out_score) {
        *out_score = score;
    }

    // FUTURO: si quieres mapear fid -> RUN real, necesitarías una tabla paralela.
    out_run.clear();
    return rc;
}

// Comparación 1:1 entre dos templates
inline int fp_match(fp_device* ctx,
                    const std::uint8_t* tpl1, unsigned int cb1,
                    const std::uint8_t* tpl2, unsigned int cb2) {
    if (!ctx || !ctx->hDB || !tpl1 || !tpl2) return -1;

    // El SDK devuelve normalmente un "score" directo en el rc
    int rc = ZKFPM_MatchFinger(
        ctx->hDB,
        const_cast<unsigned char*>(tpl1), cb1,
        const_cast<unsigned char*>(tpl2), cb2
    );
    return rc;
}