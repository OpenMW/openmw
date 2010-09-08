// Wrapper for string.h on Mac
#ifndef _STRING_WRAPPER_H
#define _STRING_WRAPPER_H

#include <string.h>
#ifdef __APPLE__
// need our own implementation of strnlen
static size_t strnlen(const char *s, size_t n)
{
  const char *p = (const char *)memchr(s, 0, n);
  return(p ? p-s : n);
}
#endif
#endif
