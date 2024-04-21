#ifndef crypto_stream_H
#define crypto_stream_H

/*
 *  WARNING: This is just a stream cipher. It is NOT authenticated encryption.
 *  While it provides some protection against eavesdropping, it does NOT
 *  provide any security against active attacks.
 *  Unless you know what you're doing, what you are looking for is probably
 *  the crypto_box functions.
 */

#include <stddef.h>

#include "sodium_crypto_stream_xsalsa20.h"
#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_stream_KEYBYTES crypto_stream_xsalsa20_KEYBYTES
size_t  crypto_stream_keybytes(void);

#define crypto_stream_NONCEBYTES crypto_stream_xsalsa20_NONCEBYTES
size_t  crypto_stream_noncebytes(void);

#define crypto_stream_MESSAGEBYTES_MAX crypto_stream_xsalsa20_MESSAGEBYTES_MAX
size_t  crypto_stream_messagebytes_max(void);

#define crypto_stream_PRIMITIVE "xsalsa20"
const char *crypto_stream_primitive(void);

int crypto_stream(unsigned char *c, unsigned long long clen,
                  const unsigned char *n, const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_stream_xor(unsigned char *c, const unsigned char *m,
                      unsigned long long mlen, const unsigned char *n,
                      const unsigned char *k)
            __attribute__ ((nonnull));

void crypto_stream_keygen(unsigned char k[crypto_stream_KEYBYTES])
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
