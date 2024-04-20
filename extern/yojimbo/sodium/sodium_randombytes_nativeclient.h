
#ifndef randombytes_nativeclient_H
#define randombytes_nativeclient_H

#ifdef __native_client__

# include "sodium_export.h"
# include "sodium_randombytes.h"

# ifdef __cplusplus
extern "C" {
# endif

extern struct randombytes_implementation randombytes_nativeclient_implementation;

# ifdef __cplusplus
}
# endif

#endif

#endif
