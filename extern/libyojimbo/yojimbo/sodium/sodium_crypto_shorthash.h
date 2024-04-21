#ifndef crypto_shorthash_H
#define crypto_shorthash_H

#include <stddef.h>

#include "sodium_crypto_shorthash_siphash24.h"
#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_shorthash_BYTES crypto_shorthash_siphash24_BYTES
size_t  crypto_shorthash_bytes(void);

#define crypto_shorthash_KEYBYTES crypto_shorthash_siphash24_KEYBYTES
size_t  crypto_shorthash_keybytes(void);

#define crypto_shorthash_PRIMITIVE "siphash24"
const char *crypto_shorthash_primitive(void);

int crypto_shorthash(unsigned char *out, const unsigned char *in,
                     unsigned long long inlen, const unsigned char *k)
            __attribute__ ((nonnull));

void crypto_shorthash_keygen(unsigned char k[crypto_shorthash_KEYBYTES])
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
