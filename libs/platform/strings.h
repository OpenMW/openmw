// Wrapper for MSVC/GCC
#ifndef _STRINGS_WRAPPER_H
#define _STRINGS_WRAPPER_H


// For GCC, just use strings.h (this applies to mingw too)
#if defined(__GNUC__)
#    include <strings.h>
#elif defined(MSVC) || defined(_MSC_VER)
#    pragma warning(disable: 4996)
#    define strcasecmp stricmp
#    define snprintf _snprintf
#else
#    warning "Unable to determine your compiler, you should probably take a look here."
#    include <strings.h>  // Just take a guess
#endif 

#endif /* _STRINGS_WRAPPER_H */
