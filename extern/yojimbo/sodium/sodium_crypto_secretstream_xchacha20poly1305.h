#ifndef crypto_secretstream_xchacha20poly1305_H
#define crypto_secretstream_xchacha20poly1305_H

#include <stddef.h>

#include "sodium_crypto_aead_xchacha20poly1305.h"
#include "sodium_crypto_stream_chacha20.h"
#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_secretstream_xchacha20poly1305_ABYTES \
    (1U + crypto_aead_xchacha20poly1305_ietf_ABYTES)
size_t crypto_secretstream_xchacha20poly1305_abytes(void);

#define crypto_secretstream_xchacha20poly1305_HEADERBYTES \
    crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
size_t crypto_secretstream_xchacha20poly1305_headerbytes(void);

#define crypto_secretstream_xchacha20poly1305_KEYBYTES \
    crypto_aead_xchacha20poly1305_ietf_KEYBYTES
size_t crypto_secretstream_xchacha20poly1305_keybytes(void);

#define crypto_secretstream_xchacha20poly1305_MESSAGEBYTES_MAX \
    SODIUM_MIN(SODIUM_SIZE_MAX - crypto_secretstream_xchacha20poly1305_ABYTES, \
              (64ULL * ((1ULL << 32) - 2ULL)))
size_t crypto_secretstream_xchacha20poly1305_messagebytes_max(void);

#define crypto_secretstream_xchacha20poly1305_TAG_MESSAGE 0x00
unsigned char crypto_secretstream_xchacha20poly1305_tag_message(void);

#define crypto_secretstream_xchacha20poly1305_TAG_PUSH    0x01
unsigned char crypto_secretstream_xchacha20poly1305_tag_push(void);

#define crypto_secretstream_xchacha20poly1305_TAG_REKEY   0x02
unsigned char crypto_secretstream_xchacha20poly1305_tag_rekey(void);

#define crypto_secretstream_xchacha20poly1305_TAG_FINAL \
    (crypto_secretstream_xchacha20poly1305_TAG_PUSH | \
     crypto_secretstream_xchacha20poly1305_TAG_REKEY)
unsigned char crypto_secretstream_xchacha20poly1305_tag_final(void);

typedef struct crypto_secretstream_xchacha20poly1305_state {
    unsigned char k[crypto_stream_chacha20_ietf_KEYBYTES];
    unsigned char nonce[crypto_stream_chacha20_ietf_NONCEBYTES];
    unsigned char _pad[8];
} crypto_secretstream_xchacha20poly1305_state;

size_t crypto_secretstream_xchacha20poly1305_statebytes(void);

void crypto_secretstream_xchacha20poly1305_keygen
   (unsigned char k[crypto_secretstream_xchacha20poly1305_KEYBYTES])
            __attribute__ ((nonnull));

int crypto_secretstream_xchacha20poly1305_init_push
   (crypto_secretstream_xchacha20poly1305_state *state,
    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES],
    const unsigned char k[crypto_secretstream_xchacha20poly1305_KEYBYTES])
            __attribute__ ((nonnull));

int crypto_secretstream_xchacha20poly1305_push
   (crypto_secretstream_xchacha20poly1305_state *state,
    unsigned char *c, unsigned long long *clen_p,
    const unsigned char *m, unsigned long long mlen,
    const unsigned char *ad, unsigned long long adlen, unsigned char tag)
            __attribute__ ((nonnull(1)));

int crypto_secretstream_xchacha20poly1305_init_pull
   (crypto_secretstream_xchacha20poly1305_state *state,
    const unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES],
    const unsigned char k[crypto_secretstream_xchacha20poly1305_KEYBYTES])
            __attribute__ ((nonnull));

int crypto_secretstream_xchacha20poly1305_pull
   (crypto_secretstream_xchacha20poly1305_state *state,
    unsigned char *m, unsigned long long *mlen_p, unsigned char *tag_p,
    const unsigned char *c, unsigned long long clen,
    const unsigned char *ad, unsigned long long adlen)
            __attribute__ ((nonnull(1)));

void crypto_secretstream_xchacha20poly1305_rekey
    (crypto_secretstream_xchacha20poly1305_state *state);

#ifdef __cplusplus
}
#endif

#endif
