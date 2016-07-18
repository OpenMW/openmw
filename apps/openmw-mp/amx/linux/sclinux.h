/*
 * Things needed to compile under linux.
 *
 * Should be reworked totally to use GNU's 'configure'
 */
#ifndef SCLINUX_H
#define SCLINUX_H

/* getchar() is not a 'cool' replacement for MSDOS getch: Linux/unix depends on the features activated or not about the
 * controlling terminal's tty. This means that ioctl(2) calls must be performed, for instance to have the controlling
 * terminal tty's in 'raw' mode, if we want to be able to fetch a single character. This also means that everything must
 * be put back correctly when the function ends. See GETCH.C for an implementation.
 *
 * For interactive use of PawnRun/PawnDbg if would be much better to use GNU's readline package: the user would be able to
 * have a complete emacs/vi like line editing system.
 */
#if !defined getch && !defined kbhit
  #include "getch.h"
#endif

#define	stricmp(a,b)    strcasecmp(a,b)
#define	strnicmp(a,b,c) strncasecmp(a,b,c)

/*
 * WinWorld wants '\'. Unices do not.
 */
#define	DIRECTORY_SEP_CHAR      '/'
#define	DIRECTORY_SEP_STR       "/"

/*
 * SC assumes that a computer is Little Endian unless told otherwise. It uses
 * (and defines) the macros BYTE_ORDER and BIG_ENDIAN.
 * For Linux, we must overrule these settings with those defined in glibc.
 */
#if !defined __BYTE_ORDER
# include <stdlib.h>
#endif

#if defined __OpenBSD__ || defined __FreeBSD__ || defined __APPLE__
# define __BYTE_ORDER    BYTE_ORDER
# define __LITTLE_ENDIAN LITTLE_ENDIAN
# define __BIG_ENDIAN    BIG_ENDIAN
#endif

#if !defined __BYTE_ORDER
# error	"Can't figure computer byte order (__BYTE_ORDER macro not found)"
#endif

#endif /* SCLINUX_H */
