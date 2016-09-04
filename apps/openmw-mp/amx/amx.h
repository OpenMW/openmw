/*  Pawn Abstract Machine (for the Pawn language)
 *
 *  Copyright (c) ITB CompuPhase, 1997-2015
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
 *  Version: $Id: amx.h 5181 2015-01-21 09:44:28Z thiadmer $
 */

#ifndef AMX_H_INCLUDED
#define AMX_H_INCLUDED

#include <stdlib.h>   /* for size_t */
#include <limits.h>

#if (defined __linux || defined __linux__) && !defined __LINUX__
  #define __LINUX__
#endif
#if defined FREEBSD && !defined __FreeBSD__
  #define __FreeBSD__
#endif
#if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
  #include <sclinux.h>
#endif

#if defined __GNUC__
 #define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
#endif

#if !defined HAVE_STDINT_H
  #if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L) \
      || defined __GNUC__ || defined __LCC__ || defined __DMC__ \
      || (defined __WATCOMC__ && __WATCOMC__ >= 1200)
    #define HAVE_STDINT_H 1
  #endif
#endif
#if !defined HAVE_INTTYPES_H
  #if defined __FreeBSD__ || defined __APPLE__
    #define HAVE_INTTYPES_H 1
  #endif
#endif
#if defined HAVE_STDINT_H
  #include <stdint.h>
#elif defined HAVE_INTTYPES_H
  #include <inttypes.h>
#else
  #if defined __MACH__
    #include <ppc/types.h>
  #endif
  typedef short int           int16_t;
  typedef unsigned short int  uint16_t;
  #if defined SN_TARGET_PS2
    typedef int               int32_t;
    typedef unsigned int      uint32_t;
  #else
    typedef long int          int32_t;
    typedef unsigned long int uint32_t;
  #endif
  #if defined __WIN32__ || defined _WIN32 || defined WIN32
    typedef __int64           int64_t;
    typedef unsigned __int64  uint64_t;
    #define HAVE_I64
  #endif
  #if !defined _INTPTR_T_DEFINED
    #if defined _LP64 || defined WIN64 || defined _WIN64
      typedef __int64         intptr_t;
    #else
      typedef int32_t         intptr_t;
    #endif
  #endif
#endif
#if defined _LP64 || defined WIN64 || defined _WIN64
  #if !defined __64BIT__
    #define __64BIT__
  #endif
#endif

#if !defined HAVE_ALLOCA_H
  #if defined __GNUC__ || defined __LCC__ || defined __DMC__ || defined __ARMCC_VERSION
    #define HAVE_ALLOCA_H 1
  #elif defined __WATCOMC__ && __WATCOMC__ >= 1200
    #define HAVE_ALLOCA_H 1
  #endif
#endif
#if defined HAVE_ALLOCA_H && HAVE_ALLOCA_H
  #include <alloca.h>
#elif defined __BORLANDC__
  #include <malloc.h>
#endif
#if defined __WIN32__ || defined _WIN32 || defined WIN32 /* || defined __MSDOS__ */
  #if !defined alloca
    #define alloca(n)   _alloca(n)
  #endif
#endif

#if !defined assert_static
  #if (defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112) || GCC_VERSION >= 40600
    #define assert_static(test) _Static_assert(test, "assert")
  #else
    /* see "Compile-Time Assertions" by Greg Miller,
     * (with modifications to port it to C)
     */
    #define _ASSERT_STATIC_SYMBOL_INNER(line) __ASSERT_STATIC_ ## line
    #define _ASSERT_STATIC_SYMBOL(line) _ASSERT_STATIC_SYMBOL_INNER(line)
    #define assert_static(test) \
      do { \
        typedef char _ASSERT_STATIC_SYMBOL(__LINE__)[ ((test) ? 1 : -1) ]; \
      } while (0)
  #endif
#endif

#if defined  __cplusplus
extern  "C" {
#endif

#if defined PAWN_DLL
  #if !defined AMX_NATIVE_CALL
    #define AMX_NATIVE_CALL __stdcall
  #endif
  #if !defined AMXAPI
    #define AMXAPI          __stdcall
  #endif
  #if !defined AMXEXPORT
    #define AMXEXPORT       __declspec(dllexport)
  #endif
#endif

/* calling convention for native functions */
#if !defined AMX_NATIVE_CALL
  #define AMX_NATIVE_CALL
#endif
/* calling convention for all interface functions and callback functions */
#if !defined AMXAPI
  #if defined STDECL
    #define AMXAPI      __stdcall
  #elif defined CDECL
    #define AMXAPI      __cdecl
  #elif defined GCC_HASCLASSVISIBILITY
    #define AMXAPI      __attribute__((visibility("default")))
  #else
    #define AMXAPI
  #endif
#endif
#if !defined AMXEXPORT
  #define AMXEXPORT
#endif

/* File format version (in CUR_FILE_VERSION)
 *   0 original version
 *   1 opcodes JUMP.pri, SWITCH and CASETBL
 *   2 compressed files
 *   3 public variables
 *   4 opcodes SWAP.pri/alt and PUSHADDR
 *   5 tagnames table
 *   6 reformatted header
 *   7 name table, opcodes SYMTAG & SYSREQ.D
 *   8 opcode BREAK, renewed debug interface
 *   9 macro opcodes
 *  10 position-independent code, overlays, packed instructions
 *  11 relocating instructions for the native interface, reorganized instruction set
 * MIN_FILE_VERSION is the lowest file version number that the current AMX
 * implementation supports. If the AMX file header gets new fields, this number
 * often needs to be incremented. MIN_AMX_VERSION is the lowest AMX version that
 * is needed to support the current file version. When there are new opcodes,
 * this number needs to be incremented.
 * The file version supported by the JIT may run behind MIN_AMX_VERSION. So
 * there is an extra constant for it: MAX_FILE_VER_JIT.
 */

#define CUR_FILE_VERSION 11     /* current file version; also the current AMX version */
#define MIN_FILE_VERSION 11     /* lowest supported file format version for the current AMX version */
#define MIN_AMX_VERSION  11     /* minimum AMX version needed to support the current file format */
#define MAX_FILE_VER_JIT 11     /* file version supported by the JIT */
#define MIN_AMX_VER_JIT  11     /* AMX version supported by the JIT */

#if !defined PAWN_CELL_SIZE
  #define PAWN_CELL_SIZE 32     /* by default, use 32-bit cells */
#endif
#if PAWN_CELL_SIZE==16
  typedef uint16_t  ucell;
  typedef int16_t   cell;
#elif PAWN_CELL_SIZE==32
  typedef uint32_t  ucell;
  typedef int32_t   cell;
#elif PAWN_CELL_SIZE==64
  typedef uint64_t  ucell;
  typedef int64_t   cell;
  #define HAVE_I64
#else
  #error Unsupported cell size (PAWN_CELL_SIZE)
#endif

#define UNPACKEDMAX   (((cell)1 << (sizeof(cell)-1)*8) - 1)
#define UNLIMITED     (~1u >> 1)

struct tagAMX;
typedef cell (AMX_NATIVE_CALL *AMX_NATIVE)(struct tagAMX *amx, const cell *params);
typedef int (AMXAPI *AMX_CALLBACK)(struct tagAMX *amx, cell index,
                                   cell *result, const cell *params);
typedef int (AMXAPI *AMX_DEBUG)(struct tagAMX *amx);
typedef int (AMXAPI *AMX_OVERLAY)(struct tagAMX *amx, int index);
typedef int (AMXAPI *AMX_IDLE)(struct tagAMX *amx, int AMXAPI Exec(struct tagAMX *, cell *, int));
#if !defined _FAR
  #define _FAR
#endif

#if defined _MSC_VER
  #pragma warning(disable:4100)  /* "'%$S' : unreferenced formal parameter" */
  #pragma warning(disable:4103)  /* disable warning message 4103 that complains
                                  * about pragma pack in a header file */
  #pragma warning(disable:4127)  /* "conditional expression is constant" (needed for static_assert) */
  #pragma warning(disable:4996)  /* POSIX name is deprecated */
#endif

/* Some compilers do not support the #pragma align, which should be fine. Some
 * compilers give a warning on unknown #pragmas, which is not so fine...
 */
#if (defined SN_TARGET_PS2 || defined __GNUC__) && !defined AMX_NO_ALIGN
  #define AMX_NO_ALIGN
#endif

#if defined __GNUC__
  #define PACKED        __attribute__((packed))
#else
  #define PACKED
#endif

#if !defined AMX_NO_ALIGN
  #if defined __LINUX__ || defined __FreeBSD__ || defined __APPLE__
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
  #elif defined MACOS && defined __MWERKS__
    #pragma options align=mac68k
  #else
    #pragma pack(push)
    #pragma pack(1)         /* structures must be packed (byte-aligned) */
    #if defined __TURBOC__
      #pragma option -a-    /* "pack" pragma for older Borland compilers */
    #endif
  #endif
#endif

typedef struct tagAMX_NATIVE_INFO {
  const char _FAR *name;
  AMX_NATIVE func;
} PACKED AMX_NATIVE_INFO;

#if !defined AMX_USERNUM
#define AMX_USERNUM     4
#endif
#define sEXPMAX         19  /* maximum name length for file version <= 6 */
#define sNAMEMAX        31  /* maximum name length of symbol name */

typedef struct tagFUNCSTUB {
  uint32_t address;
  uint32_t nameofs;
} PACKED AMX_FUNCSTUB;

typedef struct tagOVERLAYINFO {
  int32_t offset;           /* offset relative to the start of the code block */
  int32_t size;             /* size in bytes */
} PACKED AMX_OVERLAYINFO;

/* The AMX structure is the internal structure for many functions. Not all
 * fields are valid at all times; many fields are cached in local variables.
 */
typedef struct tagAMX {
  unsigned char _FAR *base; /* points to the AMX header, perhaps followed by P-code and data */
  unsigned char _FAR *code; /* points to P-code block, possibly in ROM or in an overlay pool */
  unsigned char _FAR *data; /* points to separate data+stack+heap, may be NULL */
  AMX_CALLBACK callback;    /* native function callback */
  AMX_DEBUG debug;          /* debug callback */
  AMX_OVERLAY overlay;      /* overlay reader callback */
  /* for external functions a few registers must be accessible from the outside */
  cell cip;                 /* instruction pointer: relative to base + amxhdr->cod */
  cell frm;                 /* stack frame base: relative to base + amxhdr->dat */
  cell hea;                 /* top of the heap: relative to base + amxhdr->dat */
  cell hlw;                 /* bottom of the heap: relative to base + amxhdr->dat */
  cell stk;                 /* stack pointer: relative to base + amxhdr->dat */
  cell stp;                 /* top of the stack: relative to base + amxhdr->dat */
  int flags;                /* current status, see amx_Flags() */
  /* user data */
  #if AMX_USERNUM > 0
    long usertags[AMX_USERNUM];
    void _FAR *userdata[AMX_USERNUM];
  #endif
  /* native functions can raise an error */
  int error;
  /* passing parameters requires a "count" field */
  int paramcount;
  /* the sleep opcode needs to store the full AMX status */
  cell pri;
  cell alt;
  cell reset_stk;
  cell reset_hea;
  /* extra fields for increased performance */
  cell sysreq_d;            /* relocated address/value for the SYSREQ.D opcode */
  /* fields for overlay support and JIT support */
  int ovl_index;            /* current overlay index */
  long codesize;            /* size of the overlay, or estimated memory footprint of the native code */
  #if defined AMX_JIT
    /* support variables for the JIT */
    int reloc_size;         /* required temporary buffer for relocations */
  #endif
} PACKED AMX;

/* The AMX_HEADER structure is both the memory format as the file format. The
 * structure is used internaly.
 */
typedef struct tagAMX_HEADER {
  int32_t size;             /* size of the "file" */
  uint16_t magic;           /* signature */
  char    file_version;     /* file format version */
  char    amx_version;      /* required version of the AMX */
  int16_t flags;
  int16_t defsize;          /* size of a definition record */
  int32_t cod;              /* initial value of COD - code block */
  int32_t dat;              /* initial value of DAT - data block */
  int32_t hea;              /* initial value of HEA - start of the heap */
  int32_t stp;              /* initial value of STP - stack top */
  int32_t cip;              /* initial value of CIP - the instruction pointer */
  int32_t publics;          /* offset to the "public functions" table */
  int32_t natives;          /* offset to the "native functions" table */
  int32_t libraries;        /* offset to the table of libraries */
  int32_t pubvars;          /* offset to the "public variables" table */
  int32_t tags;             /* offset to the "public tagnames" table */
  int32_t nametable;        /* offset to the name table */
  int32_t overlays;         /* offset to the overlay table */
} PACKED AMX_HEADER;

#define AMX_MAGIC_16    0xf1e2
#define AMX_MAGIC_32    0xf1e0
#define AMX_MAGIC_64    0xf1e1
#if PAWN_CELL_SIZE==16
  #define AMX_MAGIC     AMX_MAGIC_16
#elif PAWN_CELL_SIZE==32
  #define AMX_MAGIC     AMX_MAGIC_32
#elif PAWN_CELL_SIZE==64
  #define AMX_MAGIC     AMX_MAGIC_64
#endif

enum {
  AMX_ERR_NONE,
  /* reserve the first 15 error codes for exit codes of the abstract machine */
  AMX_ERR_EXIT,         /* forced exit */
  AMX_ERR_ASSERT,       /* assertion failed */
  AMX_ERR_STACKERR,     /* stack/heap collision */
  AMX_ERR_BOUNDS,       /* index out of bounds */
  AMX_ERR_MEMACCESS,    /* invalid memory access */
  AMX_ERR_INVINSTR,     /* invalid instruction */
  AMX_ERR_STACKLOW,     /* stack underflow */
  AMX_ERR_HEAPLOW,      /* heap underflow */
  AMX_ERR_CALLBACK,     /* no callback, or invalid callback */
  AMX_ERR_NATIVE,       /* native function failed */
  AMX_ERR_DIVIDE,       /* divide by zero */
  AMX_ERR_SLEEP,        /* go into sleepmode - code can be restarted */
  AMX_ERR_INVSTATE,     /* no implementation for this state, no fall-back */

  AMX_ERR_MEMORY = 16,  /* out of memory */
  AMX_ERR_FORMAT,       /* invalid file format */
  AMX_ERR_VERSION,      /* file is for a newer version of the AMX */
  AMX_ERR_NOTFOUND,     /* function not found */
  AMX_ERR_INDEX,        /* invalid index parameter (bad entry point) */
  AMX_ERR_DEBUG,        /* debugger cannot run */
  AMX_ERR_INIT,         /* AMX not initialized (or doubly initialized) */
  AMX_ERR_USERDATA,     /* unable to set user data field (table full) */
  AMX_ERR_INIT_JIT,     /* cannot initialize the JIT */
  AMX_ERR_PARAMS,       /* parameter error */
  AMX_ERR_DOMAIN,       /* domain error, expression result does not fit in range */
  AMX_ERR_GENERAL,      /* general error (unknown or unspecific error) */
  AMX_ERR_OVERLAY,      /* overlays are unsupported (JIT) or uninitialized */
};

#define AMX_FLAG_OVERLAY  0x01  /* all function calls use overlays */
#define AMX_FLAG_DEBUG    0x02  /* symbolic info. available */
#define AMX_FLAG_NOCHECKS 0x04  /* no array bounds checking; no BREAK opcodes */
#define AMX_FLAG_SLEEP    0x08  /* script uses the sleep instruction (possible re-entry or power-down mode) */
#define AMX_FLAG_CRYPT    0x10  /* file is encrypted */
#define AMX_FLAG_DSEG_INIT 0x20 /* data section is explicitly initialized */
#define AMX_FLAG_SYSREQN 0x800  /* script uses new (optimized) version of SYSREQ opcode */
#define AMX_FLAG_NTVREG 0x1000  /* all native functions are registered */
#define AMX_FLAG_JITC   0x2000  /* abstract machine is JIT compiled */
#define AMX_FLAG_VERIFY 0x4000  /* busy verifying P-code */
#define AMX_FLAG_INIT   0x8000  /* AMX has been initialized */

#define AMX_EXEC_MAIN   (-1)    /* start at program entry point */
#define AMX_EXEC_CONT   (-2)    /* continue from last address */

#define AMX_USERTAG(a,b,c,d)    ((a) | ((b)<<8) | ((long)(c)<<16) | ((long)(d)<<24))

/* for native functions that use floating point parameters, the following
 * two macros are convenient for casting a "cell" into a "float" type _without_
 * changing the bit pattern
 */
#if PAWN_CELL_SIZE==32
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((float*)&c) )  /* cell to float */
#elif PAWN_CELL_SIZE==64
  #define amx_ftoc(f)   ( * ((cell*)&f) )   /* float to cell */
  #define amx_ctof(c)   ( * ((double*)&c) ) /* cell to float */
#else
  // amx_ftoc() and amx_ctof() cannot be used
#endif

/* when a pointer cannot be stored in a cell, cells that hold relocated
 * addresses need to be expanded
 */
#if defined __64BIT__ && PAWN_CELL_SIZE<64
  #define CELLMASK      (((int64_t)1 << PAWN_CELL_SIZE) - 1)
  #define amx_Address(amx,addr) \
                        (cell*)(((int64_t)((amx)->data ? (amx)->data : (amx)->code) & ~CELLMASK) | ((int64_t)(addr) & CELLMASK))
#elif defined __32BIT__ && PAWN_CELL_SIZE<32
  #define CELLMASK      ((1L << PAWN_CELL_SIZE) - 1)
  #define amx_Address(amx,addr) \
                        (cell*)(((int32_t)((amx)->data ? (amx)->data : (amx)->code) & ~CELLMASK) | ((int32_t)(addr) & CELLMASK))
#else
  #define amx_Address(amx,addr) ((void)(amx),(cell*)(addr))
#endif

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
  /* C99: use variable-length arrays */
  #define amx_StrParam_Type(amx,param,result,type)                          \
    int result##_length_;                                                   \
    amx_StrLen(amx_Address(amx,param),&result##_length_);                   \
    char result##_vla_[(result##_length_+1)*sizeof(*(result))];             \
    (result)=(type)result##_vla_;                                           \
    amx_GetString((char*)(result),amx_Address(amx,param),                   \
                  sizeof(*(result))>1,result##_length_+1)
  #define amx_StrParam(amx,param,result) \
    amx_StrParam_Type(amx,param,result,void*)
#else
  /* macro using alloca() */
  #define amx_StrParam_Type(amx,param,result,type)                          \
    do {                                                                    \
      int result##_length_;                                                 \
      amx_StrLen(amx_Address(amx,param),&result##_length_);                 \
      if (result##_length_>0 &&                                             \
          ((result)=(type)alloca((result##_length_+1)*sizeof(*(result))))!=NULL) \
        amx_GetString((char*)(result),amx_Address(amx,param),               \
                      sizeof(*(result))>1,result##_length_+1);              \
      else (result) = NULL;                                                 \
    } while (0)
  #define amx_StrParam(amx,param,result) \
    amx_StrParam_Type(amx,param,result,void*)
#endif

uint16_t * AMXAPI amx_Align16(uint16_t *v);
uint32_t * AMXAPI amx_Align32(uint32_t *v);
#if defined _I64_MAX || defined INT64_MAX || defined HAVE_I64
  uint64_t * AMXAPI amx_Align64(uint64_t *v);
#endif
int AMXAPI amx_Allot(AMX *amx, int cells, cell **address);
int AMXAPI amx_Callback(AMX *amx, cell index, cell *result, const cell *params);
int AMXAPI amx_Cleanup(AMX *amx);
int AMXAPI amx_Clone(AMX *amxClone, AMX *amxSource, void *data);
int AMXAPI amx_Exec(AMX *amx, cell *retval, int index);
int AMXAPI amx_FindNative(AMX *amx, const char *name, int *index);
int AMXAPI amx_FindPublic(AMX *amx, const char *name, int *index);
int AMXAPI amx_FindPubVar(AMX *amx, const char *name, cell **address);
int AMXAPI amx_FindTagId(AMX *amx, cell tag_id, char *tagname);
int AMXAPI amx_Flags(AMX *amx,uint16_t *flags);
int AMXAPI amx_GetNative(AMX *amx, int index, char *name);
int AMXAPI amx_GetPublic(AMX *amx, int index, char *name, ucell *address);
int AMXAPI amx_GetPubVar(AMX *amx, int index, char *name, cell **address);
int AMXAPI amx_GetString(char *dest,const cell *source, int use_wchar, size_t size);
int AMXAPI amx_GetTag(AMX *amx, int index, char *tagname, cell *tag_id);
int AMXAPI amx_GetUserData(AMX *amx, long tag, void **ptr);
int AMXAPI amx_Init(AMX *amx, void *program);
int AMXAPI amx_InitJIT(AMX *amx, void *reloc_table, void *native_code);
int AMXAPI amx_MemInfo(AMX *amx, long *codesize, long *datasize, long *stackheap);
int AMXAPI amx_NameLength(AMX *amx, int *length);
AMX_NATIVE_INFO * AMXAPI amx_NativeInfo(const char *name, AMX_NATIVE func);
int AMXAPI amx_NumNatives(AMX *amx, int *number);
int AMXAPI amx_NumPublics(AMX *amx, int *number);
int AMXAPI amx_NumPubVars(AMX *amx, int *number);
int AMXAPI amx_NumTags(AMX *amx, int *number);
int AMXAPI amx_Push(AMX *amx, cell value);
int AMXAPI amx_PushAddress(AMX *amx, cell *address);
int AMXAPI amx_PushArray(AMX *amx, cell **address, const cell array[], int numcells);
int AMXAPI amx_PushString(AMX *amx, cell **address, const char *string, int pack, int use_wchar);
int AMXAPI amx_RaiseError(AMX *amx, int error);
int AMXAPI amx_Register(AMX *amx, const AMX_NATIVE_INFO *nativelist, int number);
int AMXAPI amx_Release(AMX *amx, cell *address);
int AMXAPI amx_SetCallback(AMX *amx, AMX_CALLBACK callback);
int AMXAPI amx_SetDebugHook(AMX *amx, AMX_DEBUG debug);
int AMXAPI amx_SetString(cell *dest, const char *source, int pack, int use_wchar, size_t size);
int AMXAPI amx_SetUserData(AMX *amx, long tag, void *ptr);
int AMXAPI amx_StrLen(const cell *cstring, int *length);
int AMXAPI amx_UTF8Check(const char *string, int *length);
int AMXAPI amx_UTF8Get(const char *string, const char **endptr, cell *value);
int AMXAPI amx_UTF8Len(const cell *cstr, int *length);
int AMXAPI amx_UTF8Put(char *string, char **endptr, int maxchars, cell value);

#if PAWN_CELL_SIZE==16
  void amx_Swap16(uint16_t *v);
#endif
#if PAWN_CELL_SIZE==32
  void amx_Swap32(uint32_t *v);
#endif
#if PAWN_CELL_SIZE==64 && (defined _I64_MAX || defined INT64_MAX || defined HAVE_I64)
  void amx_Swap64(uint64_t *v);
#endif

#if PAWN_CELL_SIZE==16
  #define amx_AlignCell(v) amx_Align16((uint16_t*)(v))
  #define amx_SwapCell(v)  amx_Swap16((uint16_t*)(v))
#elif PAWN_CELL_SIZE==32
  #define amx_AlignCell(v) amx_Align32((uint32_t*)(v))
  #define amx_SwapCell(v)  amx_Swap32((uint32_t*)(v))
#elif PAWN_CELL_SIZE==64 && (defined _I64_MAX || defined INT64_MAX || defined HAVE_I64)
  #define amx_AlignCell(v) amx_Align64((uint64_t*)(v))
  #define amx_SwapCell(v)  amx_Swap64((uint64_t*)(v))
#else
  #error Unsupported cell size
#endif

#define amx_RegisterFunc(amx, name, func) \
  amx_Register((amx), amx_NativeInfo((name),(func)), 1);

#if !defined AMX_NO_ALIGN
  #if defined __LINUX__ || defined __FreeBSD__ || defined __APPLE__
    #pragma pack()    /* reset default packing */
  #elif defined MACOS && defined __MWERKS__
    #pragma options align=reset
  #else
    #pragma pack(pop) /* reset previous packing */
  #endif
#endif

#ifdef  __cplusplus
}
#endif

#endif /* AMX_H_INCLUDED */
