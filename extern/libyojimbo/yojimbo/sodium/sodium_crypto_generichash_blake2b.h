#ifndef crypto_generichash_blake2b_H
#define crypto_generichash_blake2b_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#if defined(__IBMC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
# pragma pack(1)
#else
# pragma pack(push, 1)
#endif

typedef struct CRYPTO_ALIGN(64) crypto_generichash_blake2b_state {
    unsigned char opaque[384];
} crypto_generichash_blake2b_state;

#if defined(__IBMC__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
# pragma pack()
#else
# pragma pack(pop)
#endif

#define crypto_generichash_blake2b_BYTES_MIN     16U
size_t crypto_generichash_blake2b_bytes_min(void);

#define crypto_generichash_blake2b_BYTES_MAX     64U
size_t crypto_generichash_blake2b_bytes_max(void);

#define crypto_generichash_blake2b_BYTES         32U
size_t crypto_generichash_blake2b_bytes(void);

#define crypto_generichash_blake2b_KEYBYTES_MIN  16U
size_t crypto_generichash_blake2b_keybytes_min(void);

#define crypto_generichash_blake2b_KEYBYTES_MAX  64U
size_t crypto_generichash_blake2b_keybytes_max(void);

#define crypto_generichash_blake2b_KEYBYTES      32U
size_t crypto_generichash_blake2b_keybytes(void);

#define crypto_generichash_blake2b_SALTBYTES     16U
size_t crypto_generichash_blake2b_saltbytes(void);

#define crypto_generichash_blake2b_PERSONALBYTES 16U
size_t crypto_generichash_blake2b_personalbytes(void);

size_t crypto_generichash_blake2b_statebytes(void);

int crypto_generichash_blake2b(unsigned char *out, size_t outlen,
                               const unsigned char *in,
                               unsigned long long inlen,
                               const unsigned char *key, size_t keylen)
            __attribute__ ((nonnull(1)));

int crypto_generichash_blake2b_salt_personal(unsigned char *out, size_t outlen,
                                             const unsigned char *in,
                                             unsigned long long inlen,
                                             const unsigned char *key,
                                             size_t keylen,
                                             const unsigned char *salt,
                                             const unsigned char *personal)
            __attribute__ ((nonnull(1)));

int crypto_generichash_blake2b_init(crypto_generichash_blake2b_state *state,
                                    const unsigned char *key,
                                    const size_t keylen, const size_t outlen)
            __attribute__ ((nonnull(1)));

int crypto_generichash_blake2b_init_salt_personal(crypto_generichash_blake2b_state *state,
                                                  const unsigned char *key,
                                                  const size_t keylen, const size_t outlen,
                                                  const unsigned char *salt,
                                                  const unsigned char *personal)
            __attribute__ ((nonnull(1)));

int crypto_generichash_blake2b_update(crypto_generichash_blake2b_state *state,
                                      const unsigned char *in,
                                      unsigned long long inlen)
            __attribute__ ((nonnull));

int crypto_generichash_blake2b_final(crypto_generichash_blake2b_state *state,
                                     unsigned char *out,
                                     const size_t outlen) __attribute__ ((nonnull));

void crypto_generichash_blake2b_keygen(unsigned char k[crypto_generichash_blake2b_KEYBYTES])
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
