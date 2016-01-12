/* Text file I/O module for the Pawn Abstract Machine
 *
 *  Copyright (c) ITB CompuPhase, 2003-2015
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: amxfile.c 5181 2015-01-21 09:44:28Z thiadmer $
 */
#if defined _UNICODE || defined __UNICODE__ || defined UNICODE
# if !defined UNICODE   /* for Windows */
#   define UNICODE
# endif
# if !defined _UNICODE  /* for C library */
#   define _UNICODE
# endif
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "osdefs.h"

#if defined __BORLANDC__
#include <dir.h>
#endif
#if defined __BORLANDC__ || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined MACOS || defined __APPLE__

#include <utime.h>
#include <unistd.h>

#else
#include <sys/utime.h>
#endif
#if defined __WIN32__ || defined __MSDOS__
#include <malloc.h>
#endif
#if defined __WATCOMC__ || defined _MSC_VER
#include <direct.h>
#endif
#if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined MACOS || defined __APPLE__

#include <dirent.h>

#else
#include <io.h>
#endif

#include "amx.h"

#if defined __WIN32__ || defined _Windows
#include <windows.h>
#endif

#include "fpattern.c"

#if !defined AMXFILE_VAR
#define AMXFILE_VAR   "AMXFILE"
#endif

#if !defined sizearray
#define sizearray(a)  (sizeof(a)/sizeof((a)[0]))
#endif

#if defined _UNICODE
#include <tchar.h>
#elif !defined __T
typedef char TCHAR;
#define __T(string)   string
#define _tchmod       chmod
#define _tcscat       strcat
#define _tcschr       strchr
#define _tcscmp       strcmp
#define _tcscpy       strcpy
#define _tcsdup       strdup
#define _tcslen       strlen
#define _tcsncpy      strncpy
#define _tcsnicmp     strnicmp
#define _tcspbrk      strpbrk
#define _tcsrchr      strrchr
#define _tcstol       strtol
#define _tfopen       fopen
#define _tfputs       fputs
#define _tgetenv      getenv
#define _tremove      remove
#define _trename      rename
#if defined __APPLE__
#define _tmkdir     mkdir
#define _trmdir     rmdir
#define _tstat      stat
#define _tutime     utime
#else
#define _tmkdir     mkdir
#define _trmdir     rmdir
#define _tstat      stat
#define _tutime     utime
#endif
#endif
#if !(defined __WIN32__ || defined _WIN32 || defined WIN32)
#define _stat(n, b)  stat(n,b)
#endif
#if !defined S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#endif

#include "minIni.h"

enum filemode
{
    io_read, /* file must exist */
            io_write, /* creates a new file */
            io_readwrite, /* file must exist */
            io_append,    /* file must exist, opened for writing only and seek to the end */
};

enum seek_whence
{
    seek_start,
    seek_current,
    seek_end,
};


/* This function only stores unpacked strings. UTF-8 is used for
 * Unicode, and packed strings can only store 7-bit and 8-bit
 * character sets (ASCII, Latin-1).
 */
static size_t fgets_cell(FILE *fp, cell *string, size_t max, int utf8mode)
{
    size_t index;
    fpos_t pos;
    cell c;
    int follow, lastcr;
    cell lowmark;

    assert(sizeof(cell) >= 4);
    assert(fp != NULL);
    assert(string != NULL);
    if (max == 0)
        return 0;

    /* get the position, in case we have to back up */
    fgetpos(fp, &pos);

    index = 0;
    follow = 0;
    lowmark = 0;
    lastcr = 0;
    for (; ;)
    {
        assert(index < max);
        if (index == max - 1)
            break;                    /* string fully filled */
        if ((c = fgetc(fp)) == EOF)
        {
            if (!utf8mode || follow == 0)
                break;                  /* no more characters */
            /* If an EOF happened halfway an UTF-8 code, the string cannot be
             * UTF-8 mode, and we must restart.
             */
            index = 0;
            fsetpos(fp, &pos);
            continue;
        } /* if */

        /* 8-bit characters are unsigned */
        if (c < 0)
            c = -c;

        if (utf8mode)
        {
            if (follow > 0 && (c & 0xc0) == 0x80)
            {
                /* leader code is active, combine with earlier code */
                string[index] = (string[index] << 6) | ((unsigned char) c & 0x3f);
                if (--follow == 0)
                {
                    /* encoding a character in more bytes than is strictly needed,
                     * is not really valid UTF-8; we are strict here to increase
                     * the chance of heuristic dectection of non-UTF-8 text
                     * (JAVA writes zero bytes as a 2-byte code UTF-8, which is invalid)
                     */
                    if (string[index] < lowmark)
                        utf8mode = 0;
                    /* the code positions 0xd800--0xdfff and 0xfffe & 0xffff do not
                     * exist in UCS-4 (and hence, they do not exist in Unicode)
                     */
                    if (string[index] >= 0xd800 && string[index] <= 0xdfff
                        || string[index] == 0xfffe || string[index] == 0xffff)
                        utf8mode = 0;
                    index++;
                } /* if */
            } else if (follow == 0 && (c & 0x80) == 0x80)
            {
                /* UTF-8 leader code */
                if ((c & 0xe0) == 0xc0)
                {
                    /* 110xxxxx 10xxxxxx */
                    follow = 1;
                    lowmark = 0x80;
                    string[index] = c & 0x1f;
                } else if ((c & 0xf0) == 0xe0)
                {
                    /* 1110xxxx 10xxxxxx 10xxxxxx (16 bits, BMP plane) */
                    follow = 2;
                    lowmark = 0x800;
                    string[index] = c & 0x0f;
                } else if ((c & 0xf8) == 0xf0)
                {
                    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
                    follow = 3;
                    lowmark = 0x10000;
                    string[index] = c & 0x07;
                } else if ((c & 0xfc) == 0xf8)
                {
                    /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
                    follow = 4;
                    lowmark = 0x200000;
                    string[index] = c & 0x03;
                } else if ((c & 0xfe) == 0xfc)
                {
                    /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (31 bits) */
                    follow = 5;
                    lowmark = 0x4000000;
                    string[index] = c & 0x01;
                } else
                {
                    /* this is invalid UTF-8 */
                    utf8mode = 0;
                } /* if */
            } else if (follow == 0 && (c & 0x80) == 0x00)
            {
                /* 0xxxxxxx (US-ASCII) */
                string[index++] = c;
                if (c == __T('\n'))
                    break;        /* read newline, done */
            } else
            {
                /* this is invalid UTF-8 */
                utf8mode = 0;
            } /* if */
            if (!utf8mode)
            {
                /* UTF-8 mode was switched just off, which means that non-conforming
                 * UTF-8 codes were found, which means in turn that the string is
                 * probably not intended as UTF-8; start over again
                 */
                index = 0;
                fsetpos(fp, &pos);
            } /* if */
        } else
        {
            string[index++] = c;
            if (c == __T('\n'))
            {
                break;                  /* read newline, done */
            } else if (lastcr)
            {
                ungetc(c, fp);           /* carriage return was read, no newline follows */
                break;
            } /* if */
            lastcr = (c == __T('\r'));
        } /* if */
    } /* for */
    assert(index < max);
    string[index] = __T('\0');

    return index;
}

static size_t fputs_cell(FILE *fp, cell *string, int utf8mode)
{
    size_t count = 0;

    assert(sizeof(cell) >= 4);
    assert(fp != NULL);
    assert(string != NULL);

    while (*string != 0)
    {
        if (utf8mode)
        {
            cell c = *string;
            if (c < 0x80)
            {
                /* 0xxxxxxx */
                fputc((unsigned char) c, fp);
            } else if (c < 0x800)
            {
                /* 110xxxxx 10xxxxxx */
                fputc((unsigned char) ((c >> 6) & 0x1f | 0xc0), fp);
                fputc((unsigned char) (c & 0x3f | 0x80), fp);
            } else if (c < 0x10000)
            {
                /* 1110xxxx 10xxxxxx 10xxxxxx (16 bits, BMP plane) */
                fputc((unsigned char) ((c >> 12) & 0x0f | 0xe0), fp);
                fputc((unsigned char) ((c >> 6) & 0x3f | 0x80), fp);
                fputc((unsigned char) (c & 0x3f | 0x80), fp);
            } else if (c < 0x200000)
            {
                /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
                fputc((unsigned char) ((c >> 18) & 0x07 | 0xf0), fp);
                fputc((unsigned char) ((c >> 12) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 6) & 0x3f | 0x80), fp);
                fputc((unsigned char) (c & 0x3f | 0x80), fp);
            } else if (c < 0x4000000)
            {
                /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
                fputc((unsigned char) ((c >> 24) & 0x03 | 0xf8), fp);
                fputc((unsigned char) ((c >> 18) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 12) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 6) & 0x3f | 0x80), fp);
                fputc((unsigned char) (c & 0x3f | 0x80), fp);
            } else
            {
                /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (31 bits) */
                fputc((unsigned char) ((c >> 30) & 0x01 | 0xfc), fp);
                fputc((unsigned char) ((c >> 24) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 18) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 12) & 0x3f | 0x80), fp);
                fputc((unsigned char) ((c >> 6) & 0x3f | 0x80), fp);
                fputc((unsigned char) (c & 0x3f | 0x80), fp);
            } /* if */
        } else
        {
            /* not UTF-8 mode */
            fputc((unsigned char) *string, fp);
        } /* if */
        string++;
        count++;
    } /* while */
    return count;
}

static size_t fgets_char(FILE *fp, char *string, size_t max)
{
    size_t index;
    int c, lastcr;

    index = 0;
    lastcr = 0;
    for (; ;)
    {
        assert(index < max);
        if (index == max - 1)
            break;                    /* string fully filled */
        if ((c = fgetc(fp)) == EOF)
            break;                    /* no more characters */
        string[index++] = (char) c;
        if (c == __T('\n'))
        {
            break;                    /* read newline, done */
        } else if (lastcr)
        {
            ungetc(c, fp);             /* carriage return was read, no newline follows */
            break;
        } /* if */
        lastcr = (c == __T('\r'));
    } /* for */
    assert(index < max);
    string[index] = __T('\0');

    return index;
}

#if (defined __WIN32__ || defined _WIN32 || defined WIN32) && _MSC_VER < 1500
#if defined _UNICODE
wchar_t *_wgetenv(wchar_t *name)
{
static wchar_t buffer[_MAX_PATH];
  buffer[0]=L'\0';
  GetEnvironmentVariable(name,buffer,sizearray(buffer));
  return buffer[0]!=L'\0' ? buffer : NULL;
}
#else
char *getenv(const char *name)
{
static char buffer[_MAX_PATH];
  buffer[0]='\0';
  GetEnvironmentVariable(name,buffer,sizearray(buffer));
  return buffer[0]!='\0' ? buffer : NULL;
}
#endif
#endif

static char *completename(TCHAR *dest, TCHAR *src, size_t size)
{
#if defined AMXFILE_VAR
    TCHAR *prefix, *ptr;
    size_t len;

    /* only files below a specific path are accessible */
    prefix = getenv(AMXFILE_VAR);

    /* if no specific path for files is present, use the "temporary" path */
    if (prefix == NULL)
        prefix = getenv(__T("tmp"));    /* common under Windows and Unix */
    if (prefix == NULL)
        prefix = getenv(__T("temp"));   /* common under Windows */
    if (prefix == NULL)
        prefix = getenv(__T("tmpdir")); /* common under Unix */

    /* if no path for files is defined, and no temporary directory exists,
     * fail the function; this is for security reasons.
     */
    if (prefix == NULL)
        return NULL;

    if (_tcslen(prefix) + 1 >= size) /* +1 because directory separator is appended */
        return NULL;
    _tcscpy(dest, prefix);
    /* append a directory separator (if not already present) */
    len = _tcslen(dest);
    if (len == 0)
        return NULL;              /* empty start directory is not allowed */
    if (dest[len - 1] != __T(DIRSEP_CHAR) && dest[len - 1] != __T('/') && len + 1 < size)
    {
        dest[len] = __T(DIRSEP_CHAR);
        dest[len + 1] = __T('\0');
    } /* if */
    assert(_tcslen(dest) < size);

    /* for DOS/Windows and Unix/Linux, skip everyting up to a comma, because
     * this is used to indicate a protocol (e.g. file://C:/myfile.txt)
     */
#if DIRSEP_CHAR != ':'
    if ((ptr = _tcsrchr(src, __T(':'))) != NULL)
    {
        src = ptr + 1;              /* skip protocol/drive and colon */
        /* a "drive" specifier is sometimes ended with a vertical bar instead
         * of a colon in URL specifications
         */
        if ((ptr = _tcschr(src, __T('|'))) != NULL)
            src = ptr + 1;            /* skip drive and vertical bar */
        while (src[0] == __T(DIRSEP_CHAR) || src[0] == __T('/'))
            src++;                /* skip slashes behind the protocol/drive letter */
    } /* if */
#endif

    /* skip an initial backslash or a drive specifier in the source */
    if ((src[0] == __T(DIRSEP_CHAR) || src[0] == __T('/')) && (src[1] == __T(DIRSEP_CHAR) || src[1] == __T('/')))
    {
        /* UNC path */
        char separators[] = {__T(DIRSEP_CHAR), __T('/'), __T('\0')};
        src += 2;
        ptr = _tcspbrk(src, separators);
        if (ptr != NULL)
            src = ptr + 1;
    } else if (src[0] == __T(DIRSEP_CHAR) || src[0] == __T('/'))
    {
        /* simple path starting from the root directory */
        src++;
    } /* if */

    /* disallow any "../" specifications in the source path
     * (the check below should be stricter, but directory names with
     * trailing periods are rare anyway)
     */
    for (ptr = src; *ptr != __T('\0'); ptr++)
        if (ptr[0] == __T('.') && (ptr[1] == __T(DIRSEP_CHAR) || ptr[1] == __T('/')))
            return NULL;            /* path name is not allowed */

    /* concatenate the drive letter to the destination path */
    if (_tcslen(dest) + _tcslen(src) >= size)
        return NULL;
    _tcscat(dest, src);

    /* change forward slashes into proper directory separators */
#if DIRSEP_CHAR != '/'
    while ((ptr=_tcschr(dest,__T('/')))!=NULL)
      *ptr=__T(DIRSEP_CHAR);
#endif
    return dest;

#else
    if (_tcslen(src)>=size)
      return NULL;
    _tcscpy(dest,src);
    /* change forward slashes into proper directory separators */
#if DIRSEP_CHAR!='/'
      while ((ptr=_tcschr(dest,__T('/')))!=NULL)
        *ptr=__T(DIRSEP_CHAR);
#endif
    return dest;
#endif
}

/* File: fopen(const name[], filemode: mode) */
static cell AMX_NATIVE_CALL n_fopen(AMX *amx, const cell *params)
{
    TCHAR *attrib, *altattrib;
    TCHAR *name, fullname[_MAX_PATH];
    FILE *f = NULL;

    (void) amx;
    altattrib = NULL;
    switch (params[2] & 0x7fff)
    {
        case io_read:
            attrib = __T("rb");
            break;
        case io_write:
            attrib = __T("wb");
            break;
        case io_readwrite:
            attrib = __T("r+b");
            altattrib = __T("w+b");
            break;
        case io_append:
            attrib = __T("ab");
            break;
        default:
            return 0;
    } /* switch */

    /* get the filename */
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        f = _tfopen(fullname, attrib);
        if (f == NULL && altattrib != NULL)
            f = _tfopen(fullname, altattrib);
    } /* if */

    return (cell) f;
}

/* fclose(File: handle) */
static cell AMX_NATIVE_CALL n_fclose(AMX *amx, const cell *params)
{
    (void) amx;
    return fclose((FILE *) params[1]) == 0;
}

/* fwrite(File: handle, const string[]) */
static cell AMX_NATIVE_CALL n_fwrite(AMX *amx, const cell *params)
{
    size_t r = 0;
    cell *cptr;
    char *str;
    int len;

    (void) amx;
    cptr = amx_Address(amx, params[2]);
    amx_StrLen(cptr, &len);
    if (len == 0)
        return 0;

    if ((ucell) *cptr > UNPACKEDMAX)
    {
        /* the string is packed, write it as an ASCII/ANSI string */
        if ((str = (char *) alloca(len + 1)) != NULL)
        {
            amx_GetString(str, cptr, 0, len);
            r = fputs(str, (FILE *) params[1]);
        } /* if */
    } else
    {
        /* the string is unpacked, write it as UTF-8 */
        r = fputs_cell((FILE *) params[1], cptr, 1);
    } /* if */
    return (cell) r;
}

/* fread(File: handle, string[], size=sizeof string, bool:pack=false) */
static cell AMX_NATIVE_CALL n_fread(AMX *amx, const cell *params)
{
    size_t chars;
    int max;
    char *str;
    cell *cptr;

    max = (int) params[3];
    if (max <= 0)
        return 0;
    if (params[4])
        max *= sizeof(cell);

    cptr = amx_Address(amx, params[2]);
    str = (char *) alloca(max);
    if (str == NULL || cptr == NULL)
    {
        amx_RaiseError(amx, AMX_ERR_NATIVE);
        return 0;
    } /* if */

    if (params[4])
    {
        /* store as packed string, read an ASCII/ANSI string */
        chars = fgets_char((FILE *) params[1], str, max);
        assert(chars < max);
        amx_SetString(cptr, str, 1, 0, max);
    } else
    {
        /* store and unpacked string, interpret UTF-8 */
        chars = fgets_cell((FILE *) params[1], cptr, max, 1);
    } /* if */

    assert((int) chars < max);
    return (cell) chars;
}

/* fputchar(File: handle, value, bool:utf8 = true) */
static cell AMX_NATIVE_CALL n_fputchar(AMX *amx, const cell *params)
{
    size_t result = 0;

    (void) amx;
    if (params[3])
    {
        cell str[2];
        str[0] = params[2];
        str[1] = 0;
        result = fputs_cell((FILE *) params[1], str, 1);
    } else
    {
        fputc((int) params[2], (FILE *) params[1]);
    } /* if */
    assert(result == 0 || result == 1);
    return (cell) result;
}

/* fgetchar(File: handle, bool:utf8 = true) */
static cell AMX_NATIVE_CALL n_fgetchar(AMX *amx, const cell *params)
{
    cell str[2];
    size_t result;

    (void) amx;
    if (params[2])
    {
        result = fgets_cell((FILE *) params[1], str, 2, 1);
    } else
    {
        str[0] = fgetc((FILE *) params[1]);
        result = (str[0] != EOF);
    } /* if */
    assert(result == 0 || result == 1);
    if (result == 0)
        return EOF;
    else
        return str[0];
}

#if PAWN_CELL_SIZE == 16
#define aligncell amx_Align16
#elif PAWN_CELL_SIZE == 32
#define aligncell amx_Align32
#elif PAWN_CELL_SIZE == 64 && (defined _I64_MAX || defined HAVE_I64)
#define aligncell amx_Align64
#else
#error Unsupported cell size
#endif

/* fblockwrite(File: handle, buffer[], size=sizeof buffer) */
static cell AMX_NATIVE_CALL n_fblockwrite(AMX *amx, const cell *params)
{
    cell *cptr;
    cell count = 0;

    (void) amx;
    cptr = amx_Address(amx, params[2]);
    if (cptr != NULL)
    {
        cell max = params[3];
        ucell v;
        for (count = 0; count < max; count++)
        {
            v = (ucell) *cptr++;
            if (fwrite(aligncell(&v), sizeof(cell), 1, (FILE *) params[1]) != 1)
                break;          /* write error */
        } /* for */
    } /* if */
    return count;
}

/* fblockread(File: handle, buffer[], size=sizeof buffer) */
static cell AMX_NATIVE_CALL n_fblockread(AMX *amx, const cell *params)
{
    cell *cptr;
    cell count = 0;

    (void) amx;
    cptr = amx_Address(amx, params[2]);
    if (cptr != NULL)
    {
        cell max = params[3];
        ucell v;
        for (count = 0; count < max; count++)
        {
            if (fread(&v, sizeof(cell), 1, (FILE *) params[1]) != 1)
                break;          /* write error */
            *cptr++ = (cell) *aligncell(&v);
        } /* for */
    } /* if */
    return count;
}

/* File: ftemp() */
static cell AMX_NATIVE_CALL n_ftemp(AMX *amx, const cell *params)
{
    (void) amx;
    (void) params;
    return (cell) tmpfile();
}

/* fseek(File: handle, position, seek_whence: whence=seek_start) */
static cell AMX_NATIVE_CALL n_fseek(AMX *amx, const cell *params)
{
    int whence;
    (void) amx;
    switch (params[3])
    {
        case seek_start:
            whence = SEEK_SET;
            break;
        case seek_current:
            whence = SEEK_CUR;
            break;
        case seek_end:
            whence = SEEK_END;
            //if (params[2]>0)
            //  params[2]=-params[2];
            break;
        default:
            return 0;
    } /* switch */
    return lseek(fileno((FILE *) params[1]), params[2], whence);
}

/* bool: fremove(const name[]) */
static cell AMX_NATIVE_CALL n_fremove(AMX *amx, const cell *params)
{
    int r = 1;
    TCHAR *name, fullname[_MAX_PATH];

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        /* if this is a directory, try _trmdir() */
        struct stat stbuf;
        _tstat(fullname, &stbuf);
        if (S_ISDIR(stbuf.st_mode))
            r = _trmdir(fullname);
        else
            r = _tremove(fullname);
    } /* if */
    return r == 0;
}

/* bool: fcopy(const source[], const target[]) */
static cell AMX_NATIVE_CALL n_fcopy(AMX *amx, const cell *params)
{
    int r = 1;
    TCHAR *name, oldname[_MAX_PATH], newname[_MAX_PATH];

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(oldname, name, sizearray(oldname)) != NULL)
    {
        amx_StrParam(amx, params[2], name);
        if (name != NULL && completename(newname, name, sizearray(newname)) != NULL)
        {
#if defined __WIN32__
            r= CopyFile(oldname,newname,FALSE)==FALSE;
#else
            TCHAR cmd[2 * _MAX_PATH + 10];
            sprintf(cmd, "cp %s %s", oldname, newname);
            r = system(cmd) < 0;
#endif
        } /* if */
    } /* if */
    return r == 0;
}

/* bool: frename(const oldname[], const newname[]) */
static cell AMX_NATIVE_CALL n_frename(AMX *amx, const cell *params)
{
    int r = 1;
    TCHAR *name, oldname[_MAX_PATH], newname[_MAX_PATH];

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(oldname, name, sizearray(oldname)) != NULL)
    {
        amx_StrParam(amx, params[2], name);
        if (name != NULL && completename(newname, name, sizearray(newname)) != NULL)
            r = _trename(oldname, newname);
    } /* if */
    return r == 0;
}

/* flength(File: handle) */
static cell AMX_NATIVE_CALL n_flength(AMX *amx, const cell *params)
{
    long l, c;
    int fn = fileno((FILE *) params[1]);
    c = lseek(fn, 0, SEEK_CUR); /* save the current position */
    l = lseek(fn, 0, SEEK_END); /* return the file position at its end */
    lseek(fn, c, SEEK_SET);   /* restore the file pointer */
    (void) amx;
    return l;
}

static int match_optcopy(TCHAR *out, int outlen, const TCHAR *in, int skip)
{
    if (out == NULL || skip != 0 || outlen <= 0)
        return 0;
    _tcsncpy(out, in, outlen);
    out[outlen - 1] = '\0';
    return 1;
}

static int matchfiles(const TCHAR *path, int skip, TCHAR *out, int outlen)
{
    int count = 0;
    const TCHAR *basename;
#if DIRSEP_CHAR != '/'
    TCHAR *ptr;
#endif
#if defined __WIN32__
    HANDLE hfind;
    WIN32_FIND_DATA fd;
#else
    /* assume LINUX, FreeBSD, OpenBSD, or some other variant */
    DIR *dir;
    struct dirent *entry;
    TCHAR dirname[_MAX_PATH];
#endif

    basename = _tcsrchr(path, DIRSEP_CHAR);
    basename = (basename == NULL) ? path : basename + 1;
#if DIRSEP_CHAR != '/'
    ptr=_tcsrchr(basename,DIRSEP_CHAR);
    basename=(ptr==NULL) ? basename : ptr+1;
#endif

#if defined __WIN32__
    if ((hfind=FindFirstFile(path,&fd))!=INVALID_HANDLE_VALUE) {
      do {
        if (fpattern_match(basename,fd.cFileName,-1,FALSE)) {
          count++;
          if (match_optcopy(out,outlen,fd.cFileName,skip--))
            break;
        } /* if */
      } while (FindNextFile(hfind,&fd));
      FindClose(hfind);
    } /* if */
#else
    /* copy directory part only (zero-terminate) */
    if (basename == path)
    {
        _tcscpy(dirname, ".");
    } else
    {
        _tcsncpy(dirname, path, (int) (basename - path));
        dirname[(int) (basename - path)] = __T('\0');
    } /* if */
    if ((dir = opendir(dirname)) != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (fpattern_match(basename, entry->d_name, -1, TRUE))
            {
                count++;
                if (match_optcopy(out, outlen, entry->d_name, skip--))
                    break;
            } /* if */
        } /* while */
        closedir(dir);
    } /* if */
#endif
    return count;
}

/* fexist(const pattern[]) */
static cell AMX_NATIVE_CALL n_fexist(AMX *amx, const cell *params)
{
    int r = 0;
    TCHAR *name, fullname[_MAX_PATH];

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
        r = matchfiles(fullname, 0, NULL, 0);
    return r;
}

/* bool: fmatch(filename[], const pattern[], index=0, maxlength=sizeof filename) */
static cell AMX_NATIVE_CALL n_fmatch(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    cell *cptr;

    (void) amx;
    amx_StrParam(amx, params[2], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        if (!matchfiles(fullname, params[3], fullname, sizearray(fullname)))
        {
            fullname[0] = '\0';
        } else
        {
            /* copy the string into the destination */
            cptr = amx_Address(amx, params[1]);
            amx_SetString(cptr, fullname, 1, 0, params[4]);
        } /* if */
    } /* if */
    return fullname[0] != '\0';
}

/* bool: fstat(const name[], &size = 0, &timestamp = 0, &attrib = 0, &inode = 0) */
static cell AMX_NATIVE_CALL n_fstat(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    cell *cptr;
    int result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        struct stat stbuf;
        if (_tstat(name, &stbuf) == 0)
        {
            cptr = amx_Address(amx, params[2]);
            *cptr = stbuf.st_size;
            cptr = amx_Address(amx, params[3]);
            *cptr = (cell) stbuf.st_mtime;
            cptr = amx_Address(amx, params[4]);
            *cptr = stbuf.st_mode;  /* mode/protection bits */
            cptr = amx_Address(amx, params[5]);
            *cptr = stbuf.st_ino;   /* inode number, unique id for a file */
            result = 1;
        } /* if */
    } /* if */
    return result;
}

/* bool: fattrib(const name[], timestamp=0, attrib=0x0f) */
static cell AMX_NATIVE_CALL n_fattrib(AMX *amx, const cell *params)
{
#if !(defined __WIN32__ || defined _WIN32 || defined WIN32)
#define _utime(n, t)  utime(n,t)
#define _utimbuf     utimbuf
#endif
    TCHAR *name, fullname[_MAX_PATH] = "";
    int result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        result = 1;
        if (params[2] != 0)
        {
            struct _utimbuf times;
            times.actime = (unsigned long) params[2];
            times.modtime = (unsigned long) params[2];
            result = result && (_tutime(name, &times) == 0);
        } /* if */
        if (params[3] != 0x0f)
            result = result && (_tchmod(name, (int) params[3]) == 0);
    } /* if */
    return result;
}

/* bool: fcreatedir(const name[]) */
static cell AMX_NATIVE_CALL n_fcreatedir(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    int r = 1;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
#if defined __WIN32__ || defined __DOS__
        r=_tmkdir(fullname);
#else
        r = _tmkdir(fullname, 0755);
#endif
    } /* if */
    return r == 0;
}

/* CRC32 functions are adapted from source code from www.networkdls.com
 * The table generation routines are replaced by a hard-coded table, which
 * can be stored in Flash ROM.
 */
const unsigned long ulCRCTable[256] = // CRC Lookup Table Array
        {
                0x00000000Lu, 0x77073096Lu, 0xee0e612cLu, 0x990951baLu,
                0x076dc419Lu, 0x706af48fLu, 0xe963a535Lu, 0x9e6495a3Lu,
                0x0edb8832Lu, 0x79dcb8a4Lu, 0xe0d5e91eLu, 0x97d2d988Lu,
                0x09b64c2bLu, 0x7eb17cbdLu, 0xe7b82d07Lu, 0x90bf1d91Lu,
                0x1db71064Lu, 0x6ab020f2Lu, 0xf3b97148Lu, 0x84be41deLu,
                0x1adad47dLu, 0x6ddde4ebLu, 0xf4d4b551Lu, 0x83d385c7Lu,
                0x136c9856Lu, 0x646ba8c0Lu, 0xfd62f97aLu, 0x8a65c9ecLu,
                0x14015c4fLu, 0x63066cd9Lu, 0xfa0f3d63Lu, 0x8d080df5Lu,
                0x3b6e20c8Lu, 0x4c69105eLu, 0xd56041e4Lu, 0xa2677172Lu,
                0x3c03e4d1Lu, 0x4b04d447Lu, 0xd20d85fdLu, 0xa50ab56bLu,
                0x35b5a8faLu, 0x42b2986cLu, 0xdbbbc9d6Lu, 0xacbcf940Lu,
                0x32d86ce3Lu, 0x45df5c75Lu, 0xdcd60dcfLu, 0xabd13d59Lu,
                0x26d930acLu, 0x51de003aLu, 0xc8d75180Lu, 0xbfd06116Lu,
                0x21b4f4b5Lu, 0x56b3c423Lu, 0xcfba9599Lu, 0xb8bda50fLu,
                0x2802b89eLu, 0x5f058808Lu, 0xc60cd9b2Lu, 0xb10be924Lu,
                0x2f6f7c87Lu, 0x58684c11Lu, 0xc1611dabLu, 0xb6662d3dLu,
                0x76dc4190Lu, 0x01db7106Lu, 0x98d220bcLu, 0xefd5102aLu,
                0x71b18589Lu, 0x06b6b51fLu, 0x9fbfe4a5Lu, 0xe8b8d433Lu,
                0x7807c9a2Lu, 0x0f00f934Lu, 0x9609a88eLu, 0xe10e9818Lu,
                0x7f6a0dbbLu, 0x086d3d2dLu, 0x91646c97Lu, 0xe6635c01Lu,
                0x6b6b51f4Lu, 0x1c6c6162Lu, 0x856530d8Lu, 0xf262004eLu,
                0x6c0695edLu, 0x1b01a57bLu, 0x8208f4c1Lu, 0xf50fc457Lu,
                0x65b0d9c6Lu, 0x12b7e950Lu, 0x8bbeb8eaLu, 0xfcb9887cLu,
                0x62dd1ddfLu, 0x15da2d49Lu, 0x8cd37cf3Lu, 0xfbd44c65Lu,
                0x4db26158Lu, 0x3ab551ceLu, 0xa3bc0074Lu, 0xd4bb30e2Lu,
                0x4adfa541Lu, 0x3dd895d7Lu, 0xa4d1c46dLu, 0xd3d6f4fbLu,
                0x4369e96aLu, 0x346ed9fcLu, 0xad678846Lu, 0xda60b8d0Lu,
                0x44042d73Lu, 0x33031de5Lu, 0xaa0a4c5fLu, 0xdd0d7cc9Lu,
                0x5005713cLu, 0x270241aaLu, 0xbe0b1010Lu, 0xc90c2086Lu,
                0x5768b525Lu, 0x206f85b3Lu, 0xb966d409Lu, 0xce61e49fLu,
                0x5edef90eLu, 0x29d9c998Lu, 0xb0d09822Lu, 0xc7d7a8b4Lu,
                0x59b33d17Lu, 0x2eb40d81Lu, 0xb7bd5c3bLu, 0xc0ba6cadLu,
                0xedb88320Lu, 0x9abfb3b6Lu, 0x03b6e20cLu, 0x74b1d29aLu,
                0xead54739Lu, 0x9dd277afLu, 0x04db2615Lu, 0x73dc1683Lu,
                0xe3630b12Lu, 0x94643b84Lu, 0x0d6d6a3eLu, 0x7a6a5aa8Lu,
                0xe40ecf0bLu, 0x9309ff9dLu, 0x0a00ae27Lu, 0x7d079eb1Lu,
                0xf00f9344Lu, 0x8708a3d2Lu, 0x1e01f268Lu, 0x6906c2feLu,
                0xf762575dLu, 0x806567cbLu, 0x196c3671Lu, 0x6e6b06e7Lu,
                0xfed41b76Lu, 0x89d32be0Lu, 0x10da7a5aLu, 0x67dd4accLu,
                0xf9b9df6fLu, 0x8ebeeff9Lu, 0x17b7be43Lu, 0x60b08ed5Lu,
                0xd6d6a3e8Lu, 0xa1d1937eLu, 0x38d8c2c4Lu, 0x4fdff252Lu,
                0xd1bb67f1Lu, 0xa6bc5767Lu, 0x3fb506ddLu, 0x48b2364bLu,
                0xd80d2bdaLu, 0xaf0a1b4cLu, 0x36034af6Lu, 0x41047a60Lu,
                0xdf60efc3Lu, 0xa867df55Lu, 0x316e8eefLu, 0x4669be79Lu,
                0xcb61b38cLu, 0xbc66831aLu, 0x256fd2a0Lu, 0x5268e236Lu,
                0xcc0c7795Lu, 0xbb0b4703Lu, 0x220216b9Lu, 0x5505262fLu,
                0xc5ba3bbeLu, 0xb2bd0b28Lu, 0x2bb45a92Lu, 0x5cb36a04Lu,
                0xc2d7ffa7Lu, 0xb5d0cf31Lu, 0x2cd99e8bLu, 0x5bdeae1dLu,
                0x9b64c2b0Lu, 0xec63f226Lu, 0x756aa39cLu, 0x026d930aLu,
                0x9c0906a9Lu, 0xeb0e363fLu, 0x72076785Lu, 0x05005713Lu,
                0x95bf4a82Lu, 0xe2b87a14Lu, 0x7bb12baeLu, 0x0cb61b38Lu,
                0x92d28e9bLu, 0xe5d5be0dLu, 0x7cdcefb7Lu, 0x0bdbdf21Lu,
                0x86d3d2d4Lu, 0xf1d4e242Lu, 0x68ddb3f8Lu, 0x1fda836eLu,
                0x81be16cdLu, 0xf6b9265bLu, 0x6fb077e1Lu, 0x18b74777Lu,
                0x88085ae6Lu, 0xff0f6a70Lu, 0x66063bcaLu, 0x11010b5cLu,
                0x8f659effLu, 0xf862ae69Lu, 0x616bffd3Lu, 0x166ccf45Lu,
                0xa00ae278Lu, 0xd70dd2eeLu, 0x4e048354Lu, 0x3903b3c2Lu,
                0xa7672661Lu, 0xd06016f7Lu, 0x4969474dLu, 0x3e6e77dbLu,
                0xaed16a4aLu, 0xd9d65adcLu, 0x40df0b66Lu, 0x37d83bf0Lu,
                0xa9bcae53Lu, 0xdebb9ec5Lu, 0x47b2cf7fLu, 0x30b5ffe9Lu,
                0xbdbdf21cLu, 0xcabac28aLu, 0x53b39330Lu, 0x24b4a3a6Lu,
                0xbad03605Lu, 0xcdd70693Lu, 0x54de5729Lu, 0x23d967bfLu,
                0xb3667a2eLu, 0xc4614ab8Lu, 0x5d681b02Lu, 0x2a6f2b94Lu,
                0xb40bbe37Lu, 0xc30c8ea1Lu, 0x5a05df1bLu, 0x2d02ef8dLu
        };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function uses the ulCRCTable lookup table to generate a CRC for xData

static unsigned long PartialCRC(unsigned long ulCRC, unsigned char *sBuf, unsigned long lBufSz)
{
    while (lBufSz--)
        ulCRC = (ulCRC >> 8) ^ ulCRCTable[(ulCRC & 0xFF) ^ *sBuf++];

    return ulCRC;
}

/* filecrc(const name[]) */
static cell AMX_NATIVE_CALL n_filecrc(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    FILE *fp;
    unsigned char buffer[256];
    unsigned long ulCRC = 0xffffffff;
    size_t numread;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL
        && (fp = _tfopen(fullname, "rb")) != NULL)
    {
        do
        {
            numread = fread(buffer, sizeof(unsigned char), sizeof buffer, fp);
            ulCRC = PartialCRC(ulCRC, buffer, (unsigned long) numread);
        } while (numread == sizeof buffer);
        fclose(fp);
    } /* if */
    return (ulCRC ^ 0xffffffff);
}


const TCHAR default_ini_name[] = "config.ini";

/* readcfg(const filename[]="", const section[]="", const key[], value[], size=sizeof value, const defvalue[]="", bool:packed=false) */
static cell AMX_NATIVE_CALL n_readcfg(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    TCHAR *section, *key, *defvalue;
    TCHAR *buffer;
    int size, result = 0;
    cell *cptr;

    size = (int) params[5];
    if (size <= 0)
        return 0;
    if (params[7])
        size *= sizeof(cell);

    amx_StrParam(amx, params[1], name);
    if (name != NULL && *name == '\0')
        name = (TCHAR *) default_ini_name;
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        amx_StrParam(amx, params[2], section);
        amx_StrParam(amx, params[3], key);
        amx_StrParam(amx, params[6], defvalue);

        cptr = amx_Address(amx, params[4]);
        buffer = (char *) alloca(size);
        if (buffer == NULL || cptr == NULL)
        {
            amx_RaiseError(amx, AMX_ERR_NATIVE);
            return 0;
        } /* if */
        result = ini_gets(section, key, defvalue, buffer, size, fullname);
        amx_SetString(cptr, buffer, params[7], 0, size);
    } /* if */
    return result;
}

/* readcfgvalue(const filename[]="", const section[]="", const key[], defvalue=0) */
static cell AMX_NATIVE_CALL n_readcfgvalue(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    TCHAR *section, *key;
    long result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && *name == '\0')
        name = (TCHAR *) default_ini_name;
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        amx_StrParam(amx, params[2], section);
        amx_StrParam(amx, params[3], key);
        result = ini_getl(section, key, (long) params[4], fullname);
    } /* if */
    return result;
}

/* writecfg(const filename[]="", const section[]="", const key[], const value[]) */
static cell AMX_NATIVE_CALL n_writecfg(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    TCHAR *section, *key, *value;
    int result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && *name == '\0')
        name = (TCHAR *) default_ini_name;
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        amx_StrParam(amx, params[2], section);
        amx_StrParam(amx, params[3], key);
        amx_StrParam(amx, params[4], value);
        result = ini_puts(section, key, value, fullname);
    } /* if */
    return result;
}

/* writecfgvalue(const filename[]="", const section[]="", const key[], value) */
static cell AMX_NATIVE_CALL n_writecfgvalue(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    TCHAR *section, *key;
    int result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && *name == '\0')
        name = (TCHAR *) default_ini_name;
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        amx_StrParam(amx, params[2], section);
        amx_StrParam(amx, params[3], key);
        result = ini_putl(section, key, (long) params[4], fullname);
    } /* if */
    return result;
}

/* deletecfg(const filename[]="", const section[]="", const key[]="") */
static cell AMX_NATIVE_CALL n_deletecfg(AMX *amx, const cell *params)
{
    TCHAR *name, fullname[_MAX_PATH] = "";
    TCHAR *section, *key;
    int result = 0;

    (void) amx;
    amx_StrParam(amx, params[1], name);
    if (name != NULL && *name == '\0')
        name = (TCHAR *) default_ini_name;
    if (name != NULL && completename(fullname, name, sizearray(fullname)) != NULL)
    {
        amx_StrParam(amx, params[2], section);
        if (*section == '\0')
            section = NULL;
        amx_StrParam(amx, params[3], key);
        if (*key == '\0')
            key = NULL;
        result = ini_puts(section, key, NULL, fullname);
    } /* if */
    return result;
}

#if defined __cplusplus
extern "C"
#endif
AMX_NATIVE_INFO file_Natives[] = {
        {"fopen",         n_fopen},
        {"fclose",        n_fclose},
        {"fwrite",        n_fwrite},
        {"fread",         n_fread},
        {"fputchar",      n_fputchar},
        {"fgetchar",      n_fgetchar},
        {"fblockwrite",   n_fblockwrite},
        {"fblockread",    n_fblockread},
        {"ftemp",         n_ftemp},
        {"fseek",         n_fseek},
        {"flength",       n_flength},
        {"fremove",       n_fremove},
        {"fcopy",         n_fcopy},
        {"frename",       n_frename},
        {"fexist",        n_fexist},
        {"fmatch",        n_fmatch},
        {"fstat",         n_fstat},
        {"fattrib",       n_fattrib},
        {"filecrc",       n_filecrc},
        {"fcreatedir",    n_fcreatedir},
        {"readcfg",       n_readcfg},
        {"readcfgvalue",  n_readcfgvalue},
        {"writecfg",      n_writecfg},
        {"writecfgvalue", n_writecfgvalue},
        {"deletecfg",     n_deletecfg},
        {NULL, NULL}        /* terminator */
};

int AMXEXPORT AMXAPI amx_FileInit(AMX *amx)
{
    return amx_Register(amx, file_Natives, -1);
}

int AMXEXPORT AMXAPI amx_FileCleanup(AMX *amx)
{
    (void) amx;
    return AMX_ERR_NONE;
}
