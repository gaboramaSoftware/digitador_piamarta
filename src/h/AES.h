// encriptacion.hpp

#ifndef _AES_H_
#define _AES_H_

#include <stddef.h>
#include <stdint.h>

// configurar AES
#define AES256 1

// marco y constructores
#define AES_BLOCKLEN 16
#define AES_KEYLEN 32
#define AES_KeyExpSize 240

// estructura
struct AES_ctx {
  uint8_t RoundKey[AES_KeyExpSize];
  uint8_t Iv[AES_BLOCKLEN];
};

#ifdef __cplusplus
extern "C" {
#endif

void AES_init_ctx(struct AES_ctx *ctx, const uint8_t *key);
void AES_init_ctx_iv(struct AES_ctx *ctx, const uint8_t *key,
                     const uint8_t *iv);
void AES_ctx_set_iv(struct AES_ctx *ctx, const uint8_t *iv);

// cifrar/decifrar
void AES_CTR_xcrypt_buffer(struct AES_ctx *ctx, uint8_t *buf, uint32_t length);

#ifdef __cplusplus
}
#endif