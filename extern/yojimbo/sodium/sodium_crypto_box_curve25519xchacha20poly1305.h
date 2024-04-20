
#ifndef crypto_box_curve25519xchacha20poly1305_H
#define crypto_box_curve25519xchacha20poly1305_H

#include <stddef.h>
#include "sodium_crypto_stream_xchacha20.h"
#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

#define crypto_box_curve25519xchacha20poly1305_SEEDBYTES 32U
size_t crypto_box_curve25519xchacha20poly1305_seedbytes(void);

#define crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES 32U
size_t crypto_box_curve25519xchacha20poly1305_publickeybytes(void);

#define crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES 32U
size_t crypto_box_curve25519xchacha20poly1305_secretkeybytes(void);

#define crypto_box_curve25519xchacha20poly1305_BEFORENMBYTES 32U
size_t crypto_box_curve25519xchacha20poly1305_beforenmbytes(void);

#define crypto_box_curve25519xchacha20poly1305_NONCEBYTES 24U
size_t crypto_box_curve25519xchacha20poly1305_noncebytes(void);

#define crypto_box_curve25519xchacha20poly1305_MACBYTES 16U
size_t crypto_box_curve25519xchacha20poly1305_macbytes(void);

#define crypto_box_curve25519xchacha20poly1305_MESSAGEBYTES_MAX \
    (crypto_stream_xchacha20_MESSAGEBYTES_MAX - crypto_box_curve25519xchacha20poly1305_MACBYTES)
size_t crypto_box_curve25519xchacha20poly1305_messagebytes_max(void);

int crypto_box_curve25519xchacha20poly1305_seed_keypair(unsigned char *pk,
                                                        unsigned char *sk,
                                                        const unsigned char *seed)
            __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_keypair(unsigned char *pk,
                                                   unsigned char *sk)
            __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_easy(unsigned char *c,
                                                const unsigned char *m,
                                                unsigned long long mlen,
                                                const unsigned char *n,
                                                const unsigned char *pk,
                                                const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_open_easy(unsigned char *m,
                                                     const unsigned char *c,
                                                     unsigned long long clen,
                                                     const unsigned char *n,
                                                     const unsigned char *pk,
                                                     const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(2, 4, 5, 6)));

int crypto_box_curve25519xchacha20poly1305_detached(unsigned char *c,
                                                    unsigned char *mac,
                                                    const unsigned char *m,
                                                    unsigned long long mlen,
                                                    const unsigned char *n,
                                                    const unsigned char *pk,
                                                    const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_open_detached(unsigned char *m,
                                                         const unsigned char *c,
                                                         const unsigned char *mac,
                                                         unsigned long long clen,
                                                         const unsigned char *n,
                                                         const unsigned char *pk,
                                                         const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(2, 3, 5, 6, 7)));

/* -- Precomputation interface -- */

int crypto_box_curve25519xchacha20poly1305_beforenm(unsigned char *k,
                                                    const unsigned char *pk,
                                                    const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_easy_afternm(unsigned char *c,
                                                        const unsigned char *m,
                                                        unsigned long long mlen,
                                                        const unsigned char *n,
                                                        const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_open_easy_afternm(unsigned char *m,
                                                             const unsigned char *c,
                                                             unsigned long long clen,
                                                             const unsigned char *n,
                                                             const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(2, 4, 5)));

int crypto_box_curve25519xchacha20poly1305_detached_afternm(unsigned char *c,
                                                            unsigned char *mac,
                                                            const unsigned char *m,
                                                            unsigned long long mlen,
                                                            const unsigned char *n,
                                                            const unsigned char *k)
            __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_open_detached_afternm(unsigned char *m,
                                                                 const unsigned char *c,
                                                                 const unsigned char *mac,
                                                                 unsigned long long clen,
                                                                 const unsigned char *n,
                                                                 const unsigned char *k)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(2, 3, 5, 6)));

/* -- Ephemeral SK interface -- */

#define crypto_box_curve25519xchacha20poly1305_SEALBYTES \
    (crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES + \
     crypto_box_curve25519xchacha20poly1305_MACBYTES)

size_t crypto_box_curve25519xchacha20poly1305_sealbytes(void);

int crypto_box_curve25519xchacha20poly1305_seal(unsigned char *c,
                                                const unsigned char *m,
                                                unsigned long long mlen,
                                                const unsigned char *pk)
            __attribute__ ((nonnull));

int crypto_box_curve25519xchacha20poly1305_seal_open(unsigned char *m,
                                                     const unsigned char *c,
                                                     unsigned long long clen,
                                                     const unsigned char *pk,
                                                     const unsigned char *sk)
            __attribute__ ((warn_unused_result)) __attribute__ ((nonnull(2, 4, 5)));

#ifdef __cplusplus
}
#endif

#endif
