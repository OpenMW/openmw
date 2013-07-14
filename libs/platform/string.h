// Wrapper for string.h on Mac and MinGW
#ifndef _STRING_WRAPPER_H
#define _STRING_WRAPPER_H

#ifdef __APPLE__
#include <Availability.h>
#endif

#include <string.h>
#if (defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 1070) || defined(__MINGW32__)
// need our own implementation of strnlen
#ifdef __MINGW32__
static size_t strnlen(const char *s, size_t n)
{
    const char *p = (const char *)memchr(s, 0, n);
    return(p ? p-s : n);
}
#elif (defined(__APPLE__) && __MAC_OS_X_VERSION_MIN_REQUIRED < 1070)
static size_t mw_strnlen(const char *s, size_t n)
{
    if (strnlen != NULL) {
        return strnlen(s, n);
    }
    else {
        const char *p = (const char *)memchr(s, 0, n);
        return(p ? p-s : n);
    }
}
#define strnlen mw_strnlen
#endif

#endif

#endif
