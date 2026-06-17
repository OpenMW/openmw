
#include "sodium_crypto_secretbox.h"
#include "sodium_randombytes.h"

int
crypto_secretbox(unsigned char *c, const unsigned char *m,
                 unsigned long long mlen, const unsigned char *n,
                 const unsigned char *k)
{
    return crypto_secretbox_xsalsa20poly1305(c, m, mlen, n, k);
}

int
crypto_secretbox_open(unsigned char *m, const unsigned char *c,
                      unsigned long long clen, const unsigned char *n,
                      const unsigned char *k)
{
    return crypto_secretbox_xsalsa20poly1305_open(m, c, clen, n, k);
}

void
crypto_secretbox_keygen(unsigned char k[crypto_secretbox_KEYBYTES])
{
    randombytes_buf(k, crypto_secretbox_KEYBYTES);
}
