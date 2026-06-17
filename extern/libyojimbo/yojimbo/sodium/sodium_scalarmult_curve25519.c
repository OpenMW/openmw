
#include "sodium_crypto_scalarmult_curve25519.h"
#include "sodium_private_implementations.h"
#include "sodium_private_common.h"
#include "sodium_scalarmult_curve25519.h"
#include "sodium_runtime.h"
#include <stdio.h>

#ifdef HAVE_AVX_ASM
# include "sodium_sandy2x_curve25519.h"
#endif
#include "sodium_ref10_x25519.h"
static const crypto_scalarmult_curve25519_implementation *implementation =
    &crypto_scalarmult_curve25519_ref10_implementation;

int
crypto_scalarmult_curve25519(unsigned char *q, const unsigned char *n,
                             const unsigned char *p)
{
    size_t                 i;
    volatile unsigned char d = 0;

    if (implementation->mult(q, n, p) != 0) {
        return -1; /* LCOV_EXCL_LINE */
    }
    for (i = 0; i < crypto_scalarmult_curve25519_BYTES; i++) {
        d |= q[i];
    }
    return -(1 & ((d - 1) >> 8));
}

int
crypto_scalarmult_curve25519_base(unsigned char *q, const unsigned char *n)
{
    return implementation->mult_base(q, n);
}

size_t
crypto_scalarmult_curve25519_bytes(void)
{
    return crypto_scalarmult_curve25519_BYTES;
}

size_t
crypto_scalarmult_curve25519_scalarbytes(void)
{
    return crypto_scalarmult_curve25519_SCALARBYTES;
}

int
_crypto_scalarmult_curve25519_pick_best_implementation(void)
{
    implementation = &crypto_scalarmult_curve25519_ref10_implementation;

#ifdef HAVE_AVX_ASM
    if (sodium_runtime_has_avx()) {
        #if NETCODE_CRYPTO_LOGS
        printf( "curve25519 -> avx\n" );
        #endif // #if NETCODE_CRYPTO_LOGS
        implementation = &crypto_scalarmult_curve25519_sandy2x_implementation;
        return 0;
    }
#endif
    #if NETCODE_CRYPTO_LOGS
    printf( "curve25519 -> ref\n" );
    #endif // #if NETCODE_CRYPTO_LOGS
    return 0;
}
