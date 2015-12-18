// Wrapper for MSVC/GCC
#ifndef _STRINGS_WRAPPER_H
#define _STRINGS_WRAPPER_H


// For GCC, just use strings.h (this applies to mingw too)
#if defined(__GNUC__)
#    include <strings.h>
#elif defined(MSVC) || defined(_MSC_VER)
#    pragma warning(disable: 4996)
#    define strcasecmp stricmp
#    if (_MSC_VER < 1900)
#        include <stdio.h>
#        include <stdarg.h>
#        define snprintf c99_snprintf
#        define vsnprintf c99_vsnprintf
/* see http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010 */
inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}
#    endif
#else
#    warning "Unable to determine your compiler, you should probably take a look here."
#    include <strings.h>  // Just take a guess
#endif

#endif /* _STRINGS_WRAPPER_H */
