// Wrapper for string.h on Mac and MinGW
#ifndef _STRING_WRAPPER_H
#define _STRING_WRAPPER_H

#include <string.h>
#if defined(__APPLE__) || defined(__MINGW32__)
// need our own implementation of strnlen
static size_t strnlen(const char *s, size_t n)
{
  const char *p = (const char *)memchr(s, 0, n);
  return(p ? p-s : n);
}
#endif
#endif
