
#ifndef sodium_runtime_H
#define sodium_runtime_H

#include "sodium_export.h"

#ifdef __cplusplus
extern "C" {
#endif

int sodium_runtime_has_neon(void);

int sodium_runtime_has_sse2(void);

int sodium_runtime_has_sse3(void);

int sodium_runtime_has_ssse3(void);

int sodium_runtime_has_sse41(void);

int sodium_runtime_has_avx(void);

int sodium_runtime_has_avx2(void);

int sodium_runtime_has_avx512f(void);

int sodium_runtime_has_pclmul(void);

int sodium_runtime_has_aesni(void);

int sodium_runtime_has_rdrand(void);

/* ------------------------------------------------------------------------- */

int _sodium_runtime_get_cpu_features(void);

#ifdef __cplusplus
}
#endif

#endif
