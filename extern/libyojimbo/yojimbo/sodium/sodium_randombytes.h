
#ifndef randombytes_H
#define randombytes_H

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

#include "sodium_export.h"

#ifdef __cplusplus
# ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
extern "C" {
#endif

typedef struct randombytes_implementation {
    const char *(*implementation_name)(void); /* required */
    uint32_t    (*random)(void);              /* required */
    void        (*stir)(void);                /* optional */
    uint32_t    (*uniform)(const uint32_t upper_bound); /* optional, a default implementation will be used if NULL */
    void        (*buf)(void * const buf, const size_t size); /* required */
    int         (*close)(void);               /* optional */
} randombytes_implementation;

#define randombytes_BYTES_MAX SODIUM_MIN(SODIUM_SIZE_MAX, 0xffffffffUL)

#define randombytes_SEEDBYTES 32U
size_t randombytes_seedbytes(void);

void randombytes_buf(void * const buf, const size_t size)
            __attribute__ ((nonnull));

void randombytes_buf_deterministic(void * const buf, const size_t size,
                                   const unsigned char seed[randombytes_SEEDBYTES])
            __attribute__ ((nonnull));

uint32_t randombytes_random(void);

uint32_t randombytes_uniform(const uint32_t upper_bound);

void randombytes_stir(void);

int randombytes_close(void);

int randombytes_set_implementation(randombytes_implementation *impl)
            __attribute__ ((nonnull));

const char *randombytes_implementation_name(void);

/* -- NaCl compatibility interface -- */

void randombytes(unsigned char * const buf, const unsigned long long buf_len)
            __attribute__ ((nonnull));

#ifdef __cplusplus
}
#endif

#endif
