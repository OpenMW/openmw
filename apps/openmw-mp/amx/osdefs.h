/*
 * Platform
 *   __MSDOS__    set when compiling for DOS (not Windows)
 *   _Windows     set when compiling for any version of Microsoft Windows
 *   __WIN32__    set when compiling for Windows95 or WindowsNT (32 bit mode)
 *   __32BIT__    set when compiling in 32-bit "flat" mode (DOS, Windows, ARM)
 *   __64BIT__    set when compiling in 64-bit mode
 *   __ECOS__     set if Pawn was included with the eCos with configtool
 *   __LINUX__    set when compiling for Linux
 *
 * Copyright 1998-2011, ITB CompuPhase, The Netherlands.
 * No usage restrictions, no warranties.
 */

#ifndef _OSDEFS_H
#define _OSDEFS_H

/* Every compiler uses different "default" macros to indicate the mode
 * it is in. Throughout the source, we use the Borland C++ macros, so
 * the macros of Watcom C/C++ and Microsoft Visual C/C++ are mapped to
 * those of Borland C++.
 */
#if defined(__WATCOMC__)
  #if defined(__WINDOWS__) || defined(__NT__)
    #define _Windows    1
  #endif
  #if defined(__386__) || defined(__NT__)
    #define __32BIT__   1
  #endif
  #if defined(_Windows) && defined(__32BIT__)
    #define __WIN32__   1
  #endif
#elif defined(_MSC_VER)
  #if defined(_WINDOWS) || defined(_WIN32)
    #define _Windows    1
  #endif
  #if defined(_WIN32)
    #define __WIN32__   1
    #define __32BIT__   1
  #endif
#elif defined __arm__
  #define __32BIT__     1
#elif defined __AVR__
  #define __16BIT__     1
#endif
#if !defined __16BIT__ && !defined __32BIT__ && !defined __64BIT__
  #define __32BIT__     1
#endif


#if (defined __linux || defined __linux__) && !defined __LINUX__
  #define __LINUX__
#endif
/* To be able to eventually set __ECOS__, we have to find a symbol
 * defined in a common place (so including the header file won't break
 * anything for other platforms). <sys/types.h> includes
 * <pkgconf/system.h> and in this later file we can find CYGPKG_PAWN
 * if the Pawn package was included with configtool and so we know
 * that we are compiling for eCos.
 */
#if defined CCSINFO
  #include <sys/types.h>
#endif
#if defined CYGPKG_PAWN
  #define __ECOS__      1
  #define HAVE_ALLOCA_H 0
#endif


#if defined __FreeBSD__
  #include <sys/endian.h>
#elif defined __LINUX__
  #include <endian.h>
#elif defined __ECOS__
  #include <cyg/hal/hal_endian.h>
  #define BIG_ENDIAN    4321
  #define LITTLE_ENDIAN 1234
  #if (CYG_BYTEORDER == CYG_LSBFIRST)
  #define BYTE_ORDER  LITTLE_ENDIAN
  #else
    #define BYTE_ORDER  BIG_ENDIAN
  #endif
   /*
    * eCos option management.
    */
  #include <pkgconf/pawn.h>
  #if CYGPKG_PAWN_AMX_ANSIONLY==1
    #define AMX_ANSIONLY
  #endif
  #define PAWN_CELL_SIZE CYGPKG_PAWN_AMX_CELLSIZE
  #if CYGPKG_PAWN_CORE_RANDOM==0
    #define AMX_NORANDOM
  #endif
  #if CYGPKG_PAWN_CORE_PROPERTY==0
    #define AMX_NOPROPLIST
  #endif
  #if CYGPKG_PAWN_AMX_CONS_FIXEDPOINT==1
    #define FIXEDPOINT
  #endif
  #if CYGPKG_PAWN_AMX_CONS_FLOATPOINT==1
    #define FLOATPOINT
  #endif
#endif

/* Linux now has these */
#if !defined BIG_ENDIAN
  #define BIG_ENDIAN    4321
#endif
#if !defined LITTLE_ENDIAN
  #define LITTLE_ENDIAN 1234
#endif

/* educated guess, BYTE_ORDER is undefined, i386 is common => little endian */
#if !defined BYTE_ORDER
  #if defined UCLINUX
    #define BYTE_ORDER BIG_ENDIAN
  #else
    #define BYTE_ORDER LITTLE_ENDIAN
  #endif
#endif

#if defined __MSDOS__ || defined __WIN32__ || defined _Windows
  #define DIRSEP_CHAR '\\'
#elif defined macintosh /* only the original Macintosh uses ':', OSX uses the '/' */
  #define DIRSEP_CHAR ':'
#else
  #define DIRSEP_CHAR '/'
#endif

/* _MAX_PATH is sometimes called differently and it may be in limits.h or
 * stdlib.h instead of stdio.h.
 */
#if !defined _MAX_PATH
  /* not defined, perhaps stdio.h was not included */
  #if !defined PATH_MAX
    #include <stdio.h>
  #endif
  #if !defined _MAX_PATH && !defined PATH_MAX
    /* no _MAX_PATH and no MAX_PATH, perhaps it is in limits.h */
    #include <limits.h>
  #endif
  #if !defined _MAX_PATH && !defined PATH_MAX
    /* no _MAX_PATH and no MAX_PATH, perhaps it is in stdlib.h */
    #include <stdlib.h>
  #endif
  /* if _MAX_PATH is undefined, try common alternative names */
  #if !defined _MAX_PATH
    #if defined MAX_PATH
      #define _MAX_PATH    MAX_PATH
    #elif defined _POSIX_PATH_MAX
      #define _MAX_PATH  _POSIX_PATH_MAX
    #else
      /* everything failed, actually we have a problem here... */
      #define _MAX_PATH  1024
    #endif
  #endif
#endif

#endif  /* _OSDEFS_H */
