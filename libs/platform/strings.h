// Wrapper for MSVC
#ifndef _STRINGS_WRAPPER_H
#define _STRINGS_WRAPPER_H

#ifdef WIN32
#pragma warning(disable: 4996)
#define strcasecmp stricmp
#define snprintf _snprintf
#else
#include <strings.h>
#endif

#endif
