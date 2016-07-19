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
 *  Version: $Id: amx.c 5181 2015-01-21 09:44:28Z thiadmer $
 */

#define WIN32_LEAN_AND_MEAN
#if defined _UNICODE || defined __UNICODE__ || defined UNICODE
# if !defined UNICODE   /* for Windows API */
#   define UNICODE
# endif
# if !defined _UNICODE  /* for C library */
#   define _UNICODE
# endif
#endif

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>     /* for wchar_t */
#include <stdlib.h>     /* for getenv() */
#include <string.h>
#include "osdefs.h"
#if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
  #include <sclinux.h>
  #if !defined AMX_NODYNALOAD
    #include <dlfcn.h>
  #endif
  #if defined AMX_JIT
    #include <sys/types.h>
    #include <sys/mman.h>
  #endif
#endif
#if defined __LCC__ || defined __LINUX__
  #include <wchar.h>    /* for wcslen() */
#endif

#if defined __ECOS__
  /* eCos puts include files in cyg/package_name */
  #include <cyg/pawn/amx.h>
#else
  #include "amx.h"
#endif

#if (defined _Windows && !defined AMX_NODYNALOAD) || (defined AMX_JIT && __WIN32__)
  #include <windows.h>
#endif


/* When one or more of the AMX_funcname macros are defined, we want
 * to compile only those functions. However, when none of these macros
 * is present, we want to compile everything.
 */
#if defined AMX_ALIGN       || defined AMX_ALLOT        || defined AMX_CLEANUP
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_CLONE       || defined AMX_DEFCALLBACK  || defined AMX_EXEC
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_FLAGS       || defined AMX_INIT         || defined AMX_MEMINFO
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_NAMELENGTH   || defined AMX_NATIVEINFO  || defined AMX_PUSHXXX
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_RAISEERROR   || defined AMX_REGISTER    || defined AMX_SETCALLBACK
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_SETDEBUGHOOK || defined AMX_UTF8XXX     || defined AMX_XXXNATIVES
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_XXXPUBLICS  || defined AMX_XXXPUBVARS   || defined AMX_XXXSTRING
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if defined AMX_XXXTAGS     || defined AMX_XXXUSERDATA
  #define AMX_EXPLIT_FUNCTIONS
#endif
#if !defined AMX_EXPLIT_FUNCTIONS
  /* no constant set, set them all */
  #define AMX_ALIGN             /* amx_Align16(), amx_Align32() and amx_Align64() */
  #define AMX_ALLOT             /* amx_Allot() and amx_Release() */
  #define AMX_DEFCALLBACK       /* amx_Callback() */
  #define AMX_CLEANUP           /* amx_Cleanup() */
  #define AMX_CLONE             /* amx_Clone() */
  #define AMX_EXEC              /* amx_Exec() */
  #define AMX_FLAGS             /* amx_Flags() */
  #define AMX_INIT              /* amx_Init() and amx_InitJIT() */
  #define AMX_MEMINFO           /* amx_MemInfo() */
  #define AMX_NAMELENGTH        /* amx_NameLength() */
  #define AMX_NATIVEINFO        /* amx_NativeInfo() */
  #define AMX_PUSHXXX           /* amx_Push(), amx_PushAddress(), amx_PushArray() and amx_PushString() */
  #define AMX_RAISEERROR        /* amx_RaiseError() */
  #define AMX_REGISTER          /* amx_Register() */
  #define AMX_SETCALLBACK       /* amx_SetCallback() */
  #define AMX_SETDEBUGHOOK      /* amx_SetDebugHook() */
  #define AMX_UTF8XXX           /* amx_UTF8Check(), amx_UTF8Get(), amx_UTF8Len() and amx_UTF8Put() */
  #define AMX_XXXNATIVES        /* amx_NumNatives(), amx_GetNative() and amx_FindNative() */
  #define AMX_XXXPUBLICS        /* amx_NumPublics(), amx_GetPublic() and amx_FindPublic() */
  #define AMX_XXXPUBVARS        /* amx_NumPubVars(), amx_GetPubVar() and amx_FindPubVar() */
  #define AMX_XXXSTRING         /* amx_StrLen(), amx_GetString() and amx_SetString() */
  #define AMX_XXXTAGS           /* amx_NumTags(), amx_GetTag() and amx_FindTagId() */
  #define AMX_XXXUSERDATA       /* amx_GetUserData() and amx_SetUserData() */
#endif
#undef AMX_EXPLIT_FUNCTIONS
#if defined AMX_ANSIONLY
  #undef AMX_UTF8XXX            /* no UTF-8 support in ANSI/ASCII-only version */
#endif
#if defined AMX_NO_NATIVEINFO
  #undef AMX_NATIVEINFO
#endif
#if AMX_USERNUM <= 0
  #undef AMX_XXXUSERDATA
#endif
#if defined AMX_JIT
  /* JIT is incompatible with macro instructions, packed opcodes and overlays */
  #if !defined AMX_NO_MACRO_INSTR
    #define AMX_NO_MACRO_INSTR
  #endif
  #if !defined AMX_NO_PACKED_OPC
    #define AMX_NO_PACKED_OPC
  #endif
  #if !defined AMX_NO_OVERLAY
    #define AMX_NO_OVERLAY
  #endif
#endif
#if (defined AMX_ASM || defined AMX_JIT) && !defined AMX_ALTCORE
  /* do not use the standard ANSI-C amx_Exec() function */
  #define AMX_ALTCORE
#endif
#if !defined AMX_NO_PACKED_OPC && !defined AMX_TOKENTHREADING
  #define AMX_TOKENTHREADING    /* packed opcodes require token threading */
#endif

#if defined AMX_ALTCORE
  #if defined __WIN32__
    /* For Watcom C/C++ use register calling convention (faster); for
     * Microsoft C/C++ (and most other C compilers) use "cdecl".
     * The important point is that you assemble AMXEXEC.ASM with the matching
     * calling convention, or the right JIT, respectively.
     * AMXJITR.ASM is for Watcom's register calling convention, AMXJITS.ASM and
     * AMXJITSN.ASM are for "cdecl".
     */
    #if defined __WATCOMC__ && !defined STACKARGS
      /* register calling convention; the #pragma tells the compiler into which
       * registers the parameters should go
       */
      extern cell amx_exec_run(AMX *amx,cell *retval,unsigned char *data);
          #pragma aux amx_exec_run parm [eax] [edx] [ebx];
      extern int amx_exec_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
          #pragma aux amx_exec_run parm [eax] [edx] [ebx];
      extern cell amx_jit_compile(void *pcode, void *jumparray, void *nativecode);
          #pragma aux amx_exec_run parm [eax] [edx] [ebx];
      extern cell amx_jit_run(AMX *amx,cell *retval,unsigned char *data);
          #pragma aux amx_jit_run parm [eax] [edx] [ebx];
      extern int  amx_jit_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
          #pragma aux amx_exec_run parm [eax] [edx] [ebx];
    #elif defined __GNUC__
      /* force "cdecl" by adding an "attribute" to the declaration */
      extern cell amx_exec_run(AMX *amx,cell *retval,unsigned char *data) __attribute__((cdecl));
      extern int amx_exec_list(const AMX *amx,const cell **opcodelist,int *numopcodes) __attribute__((cdecl));
      extern cell amx_jit_compile(void *pcode, void *jumparray, void *nativecode) __attribute__((cdecl));
      extern cell amx_jit_run(AMX *amx,cell *retval,unsigned char *data) __attribute__((cdecl));
      extern int  amx_jit_list(const AMX *amx,const cell **opcodelist,int *numopcodes) __attribute__((cdecl));
    #else
      /* force "cdecl" by specifying it as a "function class" with the "__cdecl" keyword */
      extern cell __cdecl amx_exec_run(AMX *amx,cell *retval,unsigned char *data);
      extern int __cdecl amx_exec_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
      extern cell __cdecl amx_jit_compile(void *pcode, void *jumparray, void *nativecode);
      extern cell __cdecl amx_jit_run(AMX *amx,cell *retval,unsigned char *data);
      extern int  __cdecl amx_jit_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
    #endif
  #else /* __WIN32__ */
    /* assume no specific calling conventions for other platforms than Windows */
    extern cell amx_exec_run(AMX *amx,cell *retval,unsigned char *data);
    extern int amx_exec_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
    extern cell amx_jit_compile(void *pcode, void *jumparray, void *nativecode);
    extern cell amx_jit_run(AMX *amx,cell *retval,unsigned char *data);
    extern int  amx_jit_list(const AMX *amx,const cell **opcodelist,int *numopcodes);
  #endif /* __WIN32__ */
#else
  int amx_exec_list(AMX *amx,const cell **opcodelist,int *numopcodes);
#endif /* AMX_ALTCORE */

typedef enum {
  OP_NOP,
  OP_LOAD_PRI,
  OP_LOAD_ALT,
  OP_LOAD_S_PRI,
  OP_LOAD_S_ALT,
  OP_LREF_S_PRI,
  OP_LREF_S_ALT,
  OP_LOAD_I,
  OP_LODB_I,
  OP_CONST_PRI,
  OP_CONST_ALT,
  OP_ADDR_PRI,
  OP_ADDR_ALT,
  OP_STOR,
  OP_STOR_S,
  OP_SREF_S,
  OP_STOR_I,
  OP_STRB_I,
  OP_ALIGN_PRI,
  OP_LCTRL,
  OP_SCTRL,
  OP_XCHG,
  OP_PUSH_PRI,
  OP_PUSH_ALT,
  OP_PUSHR_PRI,
  OP_POP_PRI,
  OP_POP_ALT,
  OP_PICK,
  OP_STACK,
  OP_HEAP,
  OP_PROC,
  OP_RET,
  OP_RETN,
  OP_CALL,
  OP_JUMP,
  OP_JZER,
  OP_JNZ,
  OP_SHL,
  OP_SHR,
  OP_SSHR,
  OP_SHL_C_PRI,
  OP_SHL_C_ALT,
  OP_SMUL,
  OP_SDIV,
  OP_ADD,
  OP_SUB,
  OP_AND,
  OP_OR,
  OP_XOR,
  OP_NOT,
  OP_NEG,
  OP_INVERT,
  OP_EQ,
  OP_NEQ,
  OP_SLESS,
  OP_SLEQ,
  OP_SGRTR,
  OP_SGEQ,
  OP_INC_PRI,
  OP_INC_ALT,
  OP_INC_I,
  OP_DEC_PRI,
  OP_DEC_ALT,
  OP_DEC_I,
  OP_MOVS,
  OP_CMPS,
  OP_FILL,
  OP_HALT,
  OP_BOUNDS,
  OP_SYSREQ,
  OP_SWITCH,
  OP_SWAP_PRI,
  OP_SWAP_ALT,
  OP_BREAK,
  OP_CASETBL,
  /* patched instructions */
  OP_SYSREQ_D,
  OP_SYSREQ_ND,
  /* overlay instructions */
  OP_CALL_OVL,
  OP_RETN_OVL,
  OP_SWITCH_OVL,
  OP_CASETBL_OVL,
#if !defined AMX_NO_MACRO_INSTR
  /* supplemental & macro instructions */
  OP_LIDX,
  OP_LIDX_B,
  OP_IDXADDR,
  OP_IDXADDR_B,
  OP_PUSH_C,
  OP_PUSH,
  OP_PUSH_S,
  OP_PUSH_ADR,
  OP_PUSHR_C,
  OP_PUSHR_S,
  OP_PUSHR_ADR,
  OP_JEQ,
  OP_JNEQ,
  OP_JSLESS,
  OP_JSLEQ,
  OP_JSGRTR,
  OP_JSGEQ,
  OP_SDIV_INV,
  OP_SUB_INV,
  OP_ADD_C,
  OP_SMUL_C,
  OP_ZERO_PRI,
  OP_ZERO_ALT,
  OP_ZERO,
  OP_ZERO_S,
  OP_EQ_C_PRI,
  OP_EQ_C_ALT,
  OP_INC,
  OP_INC_S,
  OP_DEC,
  OP_DEC_S,
  /* macro instructions */
  OP_SYSREQ_N,
  OP_PUSHM_C,
  OP_PUSHM,
  OP_PUSHM_S,
  OP_PUSHM_ADR,
  OP_PUSHRM_C,
  OP_PUSHRM_S,
  OP_PUSHRM_ADR,
  OP_LOAD2,
  OP_LOAD2_S,
  OP_CONST,
  OP_CONST_S,
#endif
#if !defined AMX_NO_PACKED_OPC
  /* packed instructions */
  OP_LOAD_P_PRI,
  OP_LOAD_P_ALT,
  OP_LOAD_P_S_PRI,
  OP_LOAD_P_S_ALT,
  OP_LREF_P_S_PRI,
  OP_LREF_P_S_ALT,
  OP_LODB_P_I,
  OP_CONST_P_PRI,
  OP_CONST_P_ALT,
  OP_ADDR_P_PRI,
  OP_ADDR_P_ALT,
  OP_STOR_P,
  OP_STOR_P_S,
  OP_SREF_P_S,
  OP_STRB_P_I,
  OP_LIDX_P_B,
  OP_IDXADDR_P_B,
  OP_ALIGN_P_PRI,
  OP_PUSH_P_C,
  OP_PUSH_P,
  OP_PUSH_P_S,
  OP_PUSH_P_ADR,
  OP_PUSHR_P_C,
  OP_PUSHR_P_S,
  OP_PUSHR_P_ADR,
  OP_PUSHM_P_C,
  OP_PUSHM_P,
  OP_PUSHM_P_S,
  OP_PUSHM_P_ADR,
  OP_PUSHRM_P_C,
  OP_PUSHRM_P_S,
  OP_PUSHRM_P_ADR,
  OP_STACK_P,
  OP_HEAP_P,
  OP_SHL_P_C_PRI,
  OP_SHL_P_C_ALT,
  OP_ADD_P_C,
  OP_SMUL_P_C,
  OP_ZERO_P,
  OP_ZERO_P_S,
  OP_EQ_P_C_PRI,
  OP_EQ_P_C_ALT,
  OP_INC_P,
  OP_INC_P_S,
  OP_DEC_P,
  OP_DEC_P_S,
  OP_MOVS_P,
  OP_CMPS_P,
  OP_FILL_P,
  OP_HALT_P,
  OP_BOUNDS_P,
#endif
  /* ----- */
  OP_NUM_OPCODES
} OPCODE;

#define NUMENTRIES(hdr,field,nextfield) \
                        (unsigned)(((hdr)->nextfield - (hdr)->field) / (hdr)->defsize)
#define GETENTRY(hdr,table,index) \
                        (AMX_FUNCSTUB *)((unsigned char*)(hdr) + (unsigned)(hdr)->table + (unsigned)index*(hdr)->defsize)
#define GETENTRYNAME(hdr,entry) \
                        (char *)((unsigned char*)(hdr) + (unsigned)((AMX_FUNCSTUB*)(entry))->nameofs)

#if !defined NDEBUG
  static int check_endian(void)
  {
    uint16_t val=0x00ff;
    unsigned char *ptr=(unsigned char *)&val;
    /* "ptr" points to the starting address of "val". If that address
     * holds the byte "0xff", the computer stored the low byte of "val"
     * at the lower address, and so the memory lay out is Little Endian.
     */
    assert(*ptr==0xff || *ptr==0x00);
    #if BYTE_ORDER==BIG_ENDIAN
      return *ptr==0x00;  /* return "true" if big endian */
    #else
      return *ptr==0xff;  /* return "true" if little endian */
    #endif
  }
#endif

#if BYTE_ORDER==BIG_ENDIAN || PAWN_CELL_SIZE==16
  void amx_Swap16(uint16_t *v)
  {
    unsigned char *s = (unsigned char *)v;
    unsigned char t;

    assert_static(sizeof(*v)==2);
    /* swap two bytes */
    t=s[0];
    s[0]=s[1];
    s[1]=t;
  }
#endif

#if BYTE_ORDER==BIG_ENDIAN || PAWN_CELL_SIZE==32
  void amx_Swap32(uint32_t *v)
  {
    unsigned char *s = (unsigned char *)v;
    unsigned char t;

    assert_static(sizeof(*v)==4);
    /* swap outer two bytes */
    t=s[0];
    s[0]=s[3];
    s[3]=t;
    /* swap inner two bytes */
    t=s[1];
    s[1]=s[2];
    s[2]=t;
  }
#endif

#if (BYTE_ORDER==BIG_ENDIAN || PAWN_CELL_SIZE==64) && (defined _I64_MAX || defined HAVE_I64)
  void amx_Swap64(uint64_t *v)
  {
    unsigned char *s = (unsigned char *)v;
    unsigned char t;

    assert(sizeof(*v)==8);

    t=s[0];
    s[0]=s[7];
    s[7]=t;

    t=s[1];
    s[1]=s[6];
    s[6]=t;

    t=s[2];
    s[2]=s[5];
    s[5]=t;

    t=s[3];
    s[3]=s[4];
    s[4]=t;
  }
#endif

#if defined AMX_ALIGN || defined AMX_INIT
uint16_t * AMXAPI amx_Align16(uint16_t *v)
{
  assert_static(sizeof(*v)==2);
  assert(check_endian());
  #if BYTE_ORDER==BIG_ENDIAN
    amx_Swap16(v);
  #endif
  return v;
}

uint32_t * AMXAPI amx_Align32(uint32_t *v)
{
  assert_static(sizeof(*v)==4);
  assert(check_endian());
  #if BYTE_ORDER==BIG_ENDIAN
    amx_Swap32(v);
  #endif
  return v;
}

#if defined _I64_MAX || defined HAVE_I64
uint64_t * AMXAPI amx_Align64(uint64_t *v)
{
  assert(sizeof(*v)==8);
  assert(check_endian());
  #if BYTE_ORDER==BIG_ENDIAN
    amx_Swap64(v);
  #endif
  return v;
}
#endif  /* _I64_MAX || HAVE_I64 */
#endif  /* AMX_ALIGN || AMX_INIT */

#if defined AMX_FLAGS
int AMXAPI amx_Flags(AMX *amx,uint16_t *flags)
{
  AMX_HEADER *hdr;

  *flags=0;
  if (amx==NULL)
    return AMX_ERR_FORMAT;
  hdr=(AMX_HEADER *)amx->base;
  if (hdr->magic!=AMX_MAGIC)
    return AMX_ERR_FORMAT;
  if (hdr->file_version>CUR_FILE_VERSION || hdr->amx_version<MIN_FILE_VERSION)
    return AMX_ERR_VERSION;
  *flags=hdr->flags;
  return AMX_ERR_NONE;
}
#endif /* AMX_FLAGS */

#if defined AMX_DEFCALLBACK
int AMXAPI amx_Callback(AMX *amx, cell index, cell *result, const cell *params)
{
#if defined AMX_NATIVETABLE
  extern AMX_NATIVE const AMX_NATIVETABLE[];
#endif
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *func;
  AMX_NATIVE f;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->natives<=hdr->libraries);
#if defined AMX_NATIVETABLE
  if (index<0) {
    /* size of AMX_NATIVETABLE is unknown here, so we cannot verify index */
    f=(AMX_NATIVETABLE)[-(index+1)];
  } else {
#endif
    assert(index>=0 && index<(cell)NUMENTRIES(hdr,natives,libraries));
    func=GETENTRY(hdr,natives,index);
    f=(AMX_NATIVE)(intptr_t)func->address;
#if defined AMX_NATIVETABLE
  } /* if */
#endif
  assert(f!=NULL);

  /* Now that we have found the function, patch the program so that any
   * subsequent call will call the function directly (bypassing this
   * callback).
   * This trick cannot work in the JIT, because the program would need to
   * be re-JIT-compiled after patching a P-code instruction.
   */
  assert((amx->flags & AMX_FLAG_JITC)==0 || amx->sysreq_d==0);
  if (amx->sysreq_d!=0) {
    /* at the point of the call, the CIP pseudo-register points directly
     * behind the SYSREQ(.N) instruction and its parameter(s)
     */
    unsigned char *code=amx->code+(int)amx->cip-sizeof(cell);
    if (amx->flags & AMX_FLAG_SYSREQN)		/* SYSREQ.N has 2 parameters */
      code-=sizeof(cell);
    assert(amx->code!=NULL);
    assert(amx->cip>=4 && amx->cip<(hdr->dat - hdr->cod));
    assert(sizeof(f)<=sizeof(cell)); /* function pointer must fit in a cell */
    assert(*(cell*)code==index);
    #if defined AMX_TOKENTHREADING || !(defined __GNUC__ || defined __ICC || defined AMX_ASM || defined AMX_JIT)
      assert(!(amx->flags & AMX_FLAG_SYSREQN) && *(cell*)(code-sizeof(cell))==OP_SYSREQ
             || (amx->flags & AMX_FLAG_SYSREQN) && *(cell*)(code-sizeof(cell))==OP_SYSREQ_N);
    #endif
    *(cell*)(code-sizeof(cell))=amx->sysreq_d;
    *(cell*)code=(cell)(intptr_t)f;
  } /* if */

  /* Note:
   *   params[0] == number of bytes for the additional parameters passed to the native function
   *   params[1] == first argument
   *   etc.
   */

  amx->error=AMX_ERR_NONE;
  *result = f(amx,params);
  return amx->error;
}
#endif /* defined AMX_DEFCALLBACK */


#if defined AMX_JIT
  /* convert from relative addresses to absolute physical addresses */
  #define RELOC_ABS(base,off)   (*(ucell *)((base)+(int)(off)) += (ucell)(base)+(int)(off)-sizeof(cell))
#else
  #define JUMPREL(ip)           ((cell*)((intptr_t)(ip)+*(cell*)(ip)-sizeof(cell)))
#endif
#if defined AMX_ASM || defined AMX_JIT
  #define RELOCATE_ADDR(base,v) ((v)+((ucell)(base)))
#else
  #define RELOCATE_ADDR(base,v) (v)
#endif

#define DBGPARAM(v)     ( (v)=*(cell *)(amx->code+(int)cip), cip+=sizeof(cell) )

#if !defined GETOPCODE
  #if defined AMX_NO_PACKED_OPC
    #define GETOPCODE(c)  (OPCODE)(c)
  #else
    #define GETOPCODE(c)  (OPCODE)((c) & ((1 << sizeof(cell)*4)-1))
  #endif
#endif
#if !defined GETPARAM_P
  #define GETPARAM_P(v,o) ( v=((cell)(o) >> (int)(sizeof(cell)*4)) )
#endif

#if defined AMX_INIT

static int VerifyPcode(AMX *amx)
{
  AMX_HEADER *hdr;
  cell op,cip,tgt,opmask;
  int sysreq_flg,max_opcode;
  int datasize,stacksize;
  const cell *opcode_list;
  #if defined AMX_JIT
    int opcode_count=0;
    int reloc_count=0;
    int jit_codesize=0;
  #endif

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  amx->flags|=AMX_FLAG_VERIFY;
  datasize=hdr->hea-hdr->dat;
  stacksize=hdr->stp-hdr->hea;

  #if defined AMX_ASM && defined AMX_JIT
    if ((amx->flags & AMX_FLAG_JITC)!=0)
      jit_codesize=amx_jit_list(amx,&opcode_list,&max_opcode);
    else
      amx_exec_list(amx,&opcode_list,&max_opcode);
  #elif defined AMX_JIT
    jit_codesize=amx_jit_list(amx,&opcode_list,&max_opcode);
  #else
    amx_exec_list(amx,&opcode_list,&max_opcode);
  #endif
  #if defined AMX_TOKENTHREADING
    opcode_list=NULL; /* avoid token translation if token threading is in effect */
  #endif
  #if defined AMX_NO_PACKED_OPC
    opmask= ~0;
  #else
    opmask=(1 << sizeof(cell)*4)-1;
  #endif

  /* sanity checks */
  assert_static(OP_XCHG==21);
  assert_static(OP_SMUL==42);
  assert_static(OP_MOVS==64);
  #if !defined AMX_NO_MACRO_INSTR
    assert_static(OP_LIDX==81);
    assert_static(OP_ZERO_PRI==102);
    assert_static(OP_LOAD2==120);
  #endif
  #if !defined AMX_NO_PACKED_OPC
    assert_static(OP_LOAD_P_PRI==124);
    assert_static(OP_ALIGN_P_PRI==141);
    assert_static(OP_BOUNDS_P==174);
  #endif

  sysreq_flg=0;
  if (opcode_list!=NULL) {
    if (amx->sysreq_d==opcode_list[OP_SYSREQ_D])
      sysreq_flg=0x01;
    else if (amx->sysreq_d==opcode_list[OP_SYSREQ_ND])
      sysreq_flg=0x02;
  } else {
    if (amx->sysreq_d==OP_SYSREQ_D)
      sysreq_flg=0x01;
    else if (amx->sysreq_d==OP_SYSREQ_ND)
      sysreq_flg=0x02;
  } /* if */
  amx->sysreq_d=0;      /* preset */

  /* start browsing code */
  assert(amx->code!=NULL);  /* should already have been set in amx_Init() */
  for (cip=0; cip<amx->codesize; ) {
    op=*(cell *)(amx->code+(int)cip);
    if ((op & opmask)>=max_opcode) {
      amx->flags &= ~AMX_FLAG_VERIFY;
      return AMX_ERR_INVINSTR;
    } /* if */
    /* relocate opcode (only works if the size of an opcode is at least
     * as big as the size of a pointer (jump address); so basically we
     * rely on the opcode and a pointer being 32-bit
     */
    if (opcode_list!=NULL) {
      /* verify that opcode_list[op]!=NULL, if it is, this instruction
       * is unsupported
       */
      if (opcode_list[op & opmask]==0) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_INVINSTR;
      } /* if */
      *(cell *)(amx->code+(int)cip)=opcode_list[op & opmask];
    } /* if */
    #if defined AMX_JIT
      opcode_count++;
    #endif
    cip+=sizeof(cell);
    switch (op & opmask) {
#if !defined AMX_NO_MACRO_INSTR
    case OP_PUSHM_C:    /* instructions with variable number of parameters */
    case OP_PUSHM:
    case OP_PUSHM_S:
    case OP_PUSHM_ADR:
    case OP_PUSHRM_C:
    case OP_PUSHRM_S:
    case OP_PUSHRM_ADR:
      tgt=*(cell*)(amx->code+(int)cip); /* get count */
      cip+=sizeof(cell)*(tgt+1);
      break;

    case OP_LOAD2:
      tgt=*(cell*)(amx->code+(int)cip); /* verify both addresses */
      if (tgt<0 || tgt>=datasize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      tgt=*(cell*)(amx->code+(int)cip+(int)sizeof(cell));
      if (tgt<0 || tgt>=datasize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell)*2;
      break;

    case OP_LOAD2_S:
      tgt=*(cell*)(amx->code+(int)cip); /* verify both addresses */
      if (tgt<-stacksize || tgt>stacksize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      tgt=*(cell*)(amx->code+(int)cip+(int)sizeof(cell));
      if (tgt<-stacksize || tgt>stacksize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell)*2;
      break;

    case OP_CONST:
      tgt=*(cell*)(amx->code+(int)cip); /* verify address */
      if (tgt<0 || tgt>=datasize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell)*2;
      break;

    case OP_CONST_S:
      tgt=*(cell*)(amx->code+(int)cip); /* verify both addresses */
      if (tgt<-stacksize || tgt>stacksize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell)*2;
      break;
#endif /* !defined AMX_NO_MACRO_INSTR */

#if !defined AMX_NO_PACKED_OPC
    case OP_LODB_P_I:   /* instructions with 1 parameter packed inside the same cell */
    case OP_CONST_P_PRI:
    case OP_CONST_P_ALT:
    case OP_ADDR_P_PRI:
    case OP_ADDR_P_ALT:
    case OP_STRB_P_I:
    case OP_LIDX_P_B:
    case OP_IDXADDR_P_B:
    case OP_ALIGN_P_PRI:
    case OP_PUSH_P_C:
    case OP_PUSH_P:
    case OP_PUSH_P_S:
    case OP_PUSH_P_ADR:
    case OP_PUSHR_P_C:
    case OP_PUSHR_P_S:
    case OP_PUSHR_P_ADR:
    case OP_STACK_P:
    case OP_HEAP_P:
    case OP_SHL_P_C_PRI:
    case OP_SHL_P_C_ALT:
    case OP_ADD_P_C:
    case OP_SMUL_P_C:
    case OP_ZERO_P:
    case OP_ZERO_P_S:
    case OP_EQ_P_C_PRI:
    case OP_EQ_P_C_ALT:
    case OP_MOVS_P:
    case OP_CMPS_P:
    case OP_FILL_P:
    case OP_HALT_P:
    case OP_BOUNDS_P:
      break;

    case OP_LOAD_P_PRI: /* data instructions with 1 parameter packed inside the same cell */
    case OP_LOAD_P_ALT:
    case OP_STOR_P:
    case OP_INC_P:
    case OP_DEC_P:
      GETPARAM_P(tgt,op); /* verify address */
      if (tgt<0 || tgt>=datasize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      break;

    case OP_LOAD_P_S_PRI: /* stack instructions with 1 parameter packed inside the same cell */
    case OP_LOAD_P_S_ALT:
    case OP_LREF_P_S_PRI:
    case OP_LREF_P_S_ALT:
    case OP_STOR_P_S:
    case OP_SREF_P_S:
    case OP_INC_P_S:
    case OP_DEC_P_S:
      GETPARAM_P(tgt,op); /* verify address */
      if (tgt<-stacksize || tgt>stacksize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      break;

    case OP_PUSHM_P_C:    /* instructions with variable number of parameters */
    case OP_PUSHM_P:
    case OP_PUSHM_P_S:
    case OP_PUSHM_P_ADR:
    case OP_PUSHRM_P_C:
    case OP_PUSHRM_P_S:
    case OP_PUSHRM_P_ADR:
      GETPARAM_P(tgt,op); /* verify address */
      cip+=sizeof(cell)*tgt;
      break;
#endif /* !defined AMX_NO_PACKED_OPC */

    case OP_LODB_I:     /* instructions with 1 parameter (not packed) */
    case OP_CONST_PRI:
    case OP_CONST_ALT:
    case OP_ADDR_PRI:
    case OP_ADDR_ALT:
    case OP_STRB_I:
    case OP_ALIGN_PRI:
    case OP_LCTRL:
    case OP_SCTRL:
    case OP_PICK:
    case OP_STACK:
    case OP_HEAP:
    case OP_SHL_C_PRI:
    case OP_SHL_C_ALT:
    case OP_MOVS:
    case OP_CMPS:
    case OP_FILL:
    case OP_HALT:
    case OP_BOUNDS:
#if !defined AMX_NO_MACRO_INSTR
    case OP_LIDX_B:
    case OP_IDXADDR_B:
    case OP_PUSH_C:
    case OP_PUSH_ADR:
    case OP_PUSHR_C:
    case OP_PUSHR_ADR:
    case OP_ADD_C:
    case OP_SMUL_C:
    case OP_ZERO:
    case OP_ZERO_S:
    case OP_EQ_C_PRI:
    case OP_EQ_C_ALT:
#endif
      cip+=sizeof(cell);
      break;

    case OP_LOAD_PRI:
    case OP_LOAD_ALT:
    case OP_STOR:
#if !defined AMX_NO_MACRO_INSTR
    case OP_PUSH:
    case OP_INC:
    case OP_DEC:
#endif
      tgt=*(cell*)(amx->code+(int)cip); /* verify address */
      if (tgt<0 || tgt>=datasize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell);
      break;

    case OP_LOAD_S_PRI:
    case OP_LOAD_S_ALT:
    case OP_LREF_S_PRI:
    case OP_LREF_S_ALT:
    case OP_STOR_S:
    case OP_SREF_S:
#if !defined AMX_NO_MACRO_INSTR
    case OP_PUSH_S:
    case OP_PUSHR_S:
    case OP_INC_S:
    case OP_DEC_S:
#endif
      tgt=*(cell*)(amx->code+(int)cip); /* verify address */
      if (tgt<-stacksize || tgt>stacksize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      cip+=sizeof(cell);
      break;

    case OP_NOP:        /* instructions without parameters */
    case OP_LOAD_I:
    case OP_STOR_I:
    case OP_XCHG:
    case OP_PUSH_PRI:
    case OP_PUSH_ALT:
    case OP_PUSHR_PRI:
    case OP_POP_PRI:
    case OP_POP_ALT:
    case OP_PROC:
    case OP_RET:
    case OP_RETN:
    case OP_SHL:
    case OP_SHR:
    case OP_SSHR:
    case OP_SMUL:
    case OP_SDIV:
    case OP_ADD:
    case OP_SUB:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_NOT:
    case OP_NEG:
    case OP_INVERT:
    case OP_EQ:
    case OP_NEQ:
    case OP_SLESS:
    case OP_SLEQ:
    case OP_SGRTR:
    case OP_SGEQ:
    case OP_INC_PRI:
    case OP_INC_ALT:
    case OP_INC_I:
    case OP_DEC_PRI:
    case OP_DEC_ALT:
    case OP_DEC_I:
    case OP_SWAP_PRI:
    case OP_SWAP_ALT:
    case OP_BREAK:
#if !defined AMX_NO_MACRO_INSTR
    case OP_LIDX:
    case OP_IDXADDR:
    case OP_SDIV_INV:
    case OP_SUB_INV:
    case OP_ZERO_PRI:
    case OP_ZERO_ALT:
#endif
      break;

    case OP_CALL:       /* opcodes that need relocation (JIT only), or conversion to position-independent code */
    case OP_JUMP:
    case OP_JZER:
    case OP_JNZ:
    case OP_SWITCH:
#if !defined AMX_NO_MACRO_INSTR
    case OP_JEQ:
    case OP_JNEQ:
    case OP_JSLESS:
    case OP_JSLEQ:
    case OP_JSGRTR:
    case OP_JSGEQ:
#endif
      tgt=*(cell*)(amx->code+(int)cip)+cip-sizeof(cell);
      if (tgt<0 || tgt>amx->codesize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      #if defined AMX_JIT
        reloc_count++;
        RELOC_ABS(amx->code, cip);  /* change to absolute physical address */
      #endif
      cip+=sizeof(cell);
      break;

#if !defined AMX_NO_OVERLAY
    /* overlay opcodes (overlays must be enabled) */
    case OP_SWITCH_OVL:
      assert(hdr->file_version>=10);
      tgt=*(cell*)(amx->code+(int)cip)+cip-sizeof(cell);
      if (tgt<0 || tgt>amx->codesize) {
        amx->flags &= ~AMX_FLAG_VERIFY;
        return AMX_ERR_BOUNDS;
      } /* if */
      /* drop through */
    case OP_CALL_OVL:
      cip+=sizeof(cell);
      /* drop through */
    case OP_RETN_OVL:
      assert(hdr->overlays!=0 && hdr->overlays!=hdr->nametable);
      #if defined AMX_JIT
        if ((amx->flags & AMX_FLAG_JITC)!=0)
          return AMX_ERR_OVERLAY;     /* JIT does not support overlays */
      #endif
      if (amx->overlay==NULL)
        return AMX_ERR_OVERLAY;       /* no overlay callback */
      break;
    case OP_CASETBL_OVL: {
      cell num;
      DBGPARAM(num);    /* number of records follows the opcode */
      cip+=(2*num + 1)*sizeof(cell);
      if (amx->overlay==NULL)
        return AMX_ERR_OVERLAY;       /* no overlay callback */
      break;
    } /* case */
#endif

    case OP_SYSREQ:
      cip+=sizeof(cell);
      sysreq_flg|=0x01; /* mark SYSREQ found */
      break;
#if !defined AMX_NO_MACRO_INSTR
    case OP_SYSREQ_N:
      cip+=sizeof(cell)*2;
      sysreq_flg|=0x02; /* mark SYSREQ.N found */
      break;
#endif

    case OP_CASETBL: {
      cell num,offs;
      int i;
      DBGPARAM(num);    /* number of records follows the opcode */
      for (i=0; i<=num; i++) {
        offs=cip+2*i*sizeof(cell);
        tgt=*(cell*)(amx->code+(int)offs)+offs-sizeof(cell);
        if (tgt<0 || tgt>amx->codesize) {
          amx->flags &= ~AMX_FLAG_VERIFY;
          return AMX_ERR_BOUNDS;
        } /* if */
        #if defined AMX_JIT
          RELOC_ABS(amx->code, cip+2*i*sizeof(cell));
          reloc_count++;
        #endif
      } /* for */
      cip+=(2*num + 1)*sizeof(cell);
      break;
    } /* case */

    default:
      amx->flags &= ~AMX_FLAG_VERIFY;
      return AMX_ERR_INVINSTR;
    } /* switch */
  } /* for */

  #if !defined AMX_DONT_RELOCATE
    /* only either type of system request opcode should be found (otherwise,
     * we probably have a non-conforming compiler
     */
    if ((sysreq_flg==0x01 || sysreq_flg==0x02) && (amx->flags & AMX_FLAG_JITC)==0) {
      /* to use direct system requests, a function pointer must fit in a cell;
       * because the native function's address will be stored as the parameter
       * of SYSREQ.(N)D
       */
      if (sizeof(AMX_NATIVE)<=sizeof(cell)) {
        if (opcode_list!=NULL)
          amx->sysreq_d=(sysreq_flg==0x01) ? opcode_list[OP_SYSREQ_D] : opcode_list[OP_SYSREQ_ND];
        else
          amx->sysreq_d=(sysreq_flg==0x01) ? OP_SYSREQ_D : OP_SYSREQ_ND;
      } /* if */
    } /* if */
  #endif

  #if defined AMX_JIT
    /* adjust the code size to mean: estimated code size of the native code
     * (instead of the size of the P-code)
     */
    amx->codesize=jit_codesize*opcode_count + hdr->cod + (hdr->stp - hdr->dat);
    amx->reloc_size=2*sizeof(cell)*reloc_count;
  #endif

  amx->flags &= ~AMX_FLAG_VERIFY;
  amx->flags |= AMX_FLAG_INIT;
  if (sysreq_flg & 0x02)
    amx->flags |= AMX_FLAG_SYSREQN;

  return AMX_ERR_NONE;
}

/* definitions used for amx_Init() and amx_Cleanup() */
#if (defined _Windows || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__) && !defined AMX_NODYNALOAD
  typedef int AMXEXPORT (AMXAPI _FAR *AMX_ENTRY)(AMX _FAR *amx);
#endif

int AMXAPI amx_Init(AMX *amx,void *program)
{
  AMX_HEADER *hdr;
  int err;
  uint16_t *namelength;
  unsigned char *data;
  #if (defined _Windows || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__) && !defined AMX_NODYNALOAD
    #if defined _Windows
      char libname[sNAMEMAX+8]; /* +1 for '\0', +3 for 'amx' prefix, +4 for extension */
      HINSTANCE hlib;
    #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
      char libname[_MAX_PATH];
      char *root;
      void *hlib;
      #if !defined AMX_LIBPATH
        #define AMX_LIBPATH     "AMXLIB"
      #endif
    #endif
    int numlibraries;
    AMX_FUNCSTUB *lib;
    AMX_ENTRY libinit;
  #endif

  if ((amx->flags & AMX_FLAG_INIT)!=0)
    return AMX_ERR_INIT;  /* already initialized (may not do so twice) */

  hdr=(AMX_HEADER *)program;
  /* the header is in Little Endian, on a Big Endian machine, swap all
   * multi-byte words
   */
  assert(check_endian());
  #if BYTE_ORDER==BIG_ENDIAN
    amx_Align32((uint32_t*)&hdr->size);
    amx_Align16(&hdr->magic);
    amx_Align16((uint16_t*)&hdr->flags);
    amx_Align16((uint16_t*)&hdr->defsize);
    amx_Align32((uint32_t*)&hdr->cod);
    amx_Align32((uint32_t*)&hdr->dat);
    amx_Align32((uint32_t*)&hdr->hea);
    amx_Align32((uint32_t*)&hdr->stp);
    amx_Align32((uint32_t*)&hdr->cip);
    amx_Align32((uint32_t*)&hdr->publics);
    amx_Align32((uint32_t*)&hdr->natives);
    amx_Align32((uint32_t*)&hdr->libraries);
    amx_Align32((uint32_t*)&hdr->pubvars);
    amx_Align32((uint32_t*)&hdr->tags);
    if (hdr->file_version>=10)
      amx_Align32((uint32_t*)&hdr->overlays);
  #endif

  if (hdr->magic!=AMX_MAGIC)
    return AMX_ERR_FORMAT;
  if (hdr->file_version>CUR_FILE_VERSION || hdr->amx_version<MIN_FILE_VERSION)
    return AMX_ERR_VERSION;
  if (hdr->defsize!=sizeof(AMX_FUNCSTUB))
    return AMX_ERR_FORMAT;
  /* check the maximum name length in the separate name table */
  amx_Align32((uint32_t*)&hdr->nametable);
  namelength=(uint16_t*)((unsigned char*)program + (unsigned)hdr->nametable);
  amx_Align16(namelength);
  if (*namelength>sNAMEMAX)
    return AMX_ERR_FORMAT;
  if (hdr->stp<=0)
    return AMX_ERR_FORMAT;
  assert(hdr->hea == hdr->size);
  #if BYTE_ORDER==BIG_ENDIAN
    if ((hdr->flags & AMX_FLAG_COMPACT)==0) {
      ucell *code=(ucell *)((unsigned char *)program+(int)hdr->cod);
      while (code<(ucell *)((unsigned char *)program+(int)hdr->hea))
        amx_SwapCell(code++);
    } /* if */
  #endif

  amx->base=(unsigned char *)program;

  /* set initial values */
  amx->hlw=hdr->hea - hdr->dat; /* stack and heap relative to data segment */
  amx->stp=hdr->stp - hdr->dat - sizeof(cell);
  amx->hea=amx->hlw;
  amx->stk=amx->stp;
  #if defined AMX_DEFCALLBACK
    if (amx->callback==NULL)
      amx->callback=amx_Callback;
  #endif

  /* when running P-code from ROM (with the header with the native function
   * table in RAM), the "code" field must be set to a non-NULL value on
   * initialization, before calling amx_Init(); in an overlay scheme, the
   * code field is modified dynamically by the overlay callback
   */
  if (amx->code==NULL)
    amx->code=amx->base+(int)hdr->cod;
  if (amx->codesize==0)
    amx->codesize=hdr->dat-hdr->cod;

  /* to split the data segment off the code segment, the "data" field must
   * be set to a non-NULL value on initialization, before calling amx_Init();
   * you may also need to explicitly initialize the data section with the
   * contents read from the AMX file
   */
  if (amx->data!=NULL) {
    data=amx->data;
    if ((amx->flags & AMX_FLAG_DSEG_INIT)==0 && amx->overlay==NULL)
      memcpy(data,amx->base+(int)hdr->dat,(size_t)(hdr->hea-hdr->dat));
  } else {
    data=amx->base+(int)hdr->dat;
  } /* if */

  /* Set a zero cell at the top of the stack, which functions
   * as a sentinel for strings.
   */
  * (cell *)(data+(int)(hdr->stp-hdr->dat-sizeof(cell)))=0;

  /* also align all addresses in the public function, public variable,
   * public tag and native function tables --offsets into the name table
   * (if present) must also be swapped.
   */
  #if BYTE_ORDER==BIG_ENDIAN
  { /* local */
    AMX_FUNCSTUB *fs;
    int i,num;

    fs=GETENTRY(hdr,natives,0);
    num=NUMENTRIES(hdr,natives,libraries);
    for (i=0; i<num; i++) {
      amx_Align32(&fs->address);  /* redundant, because it should be zero */
      amx_Align32(&fs->nameofs);
      fs=(AMX_FUNCSTUB*)((unsigned char *)fs+hdr->defsize);
    } /* for */

    fs=GETENTRY(hdr,publics,0);
    assert(hdr->publics<=hdr->natives);
    num=NUMENTRIES(hdr,publics,natives);
    for (i=0; i<num; i++) {
      amx_Align32(&fs->address);
      amx_Align32(&fs->nameofs);
      fs=(AMX_FUNCSTUB*)((unsigned char *)fs+hdr->defsize);
    } /* for */

    fs=GETENTRY(hdr,pubvars,0);
    assert(hdr->pubvars<=hdr->tags);
    num=NUMENTRIES(hdr,pubvars,tags);
    for (i=0; i<num; i++) {
      amx_Align32(&fs->address);
      amx_Align32(&fs->nameofs);
      fs=(AMX_FUNCSTUB*)((unsigned char *)fs+hdr->defsize);
    } /* for */

    fs=GETENTRY(hdr,tags,0);
    if (hdr->file_version<7) {  /* file version 7 introduced the name table */
      assert(hdr->tags<=hdr->cod);
      num=NUMENTRIES(hdr,tags,cod);
    } else {
      assert(hdr->tags<=hdr->nametable);
      num=NUMENTRIES(hdr,tags,nametable);
    } /* if */
    for (i=0; i<num; i++) {
      amx_Align32(&fs->address);
      amx_Align32(&fs->nameofs);
      fs=(AMX_FUNCSTUB*)((unsigned char *)fs+hdr->defsize);
    } /* for */
  } /* local */
  #endif

  /* verify P-code and relocate address in the case of the JIT */
  if ((hdr->flags & AMX_FLAG_OVERLAY)==0) {
    err=VerifyPcode(amx);
  } else {
    int i;
    err=(amx->overlay==NULL) ? AMX_ERR_OVERLAY : AMX_ERR_NONE;
    /* load every overlay on initialization and verify explicitly; we must
     * do this to know whether to use new or old system requests
     */
    for (i=0; err==AMX_ERR_NONE && i<(int)((hdr->nametable - hdr->overlays)/sizeof(AMX_OVERLAYINFO)); i++) {
      err=amx->overlay(amx, i);
      if (err==AMX_ERR_NONE)
        err=VerifyPcode(amx);
    } /* for */
  } /* if */
  if (err!=AMX_ERR_NONE)
    return err;

  /* load any extension modules that the AMX refers to */
  #if (defined _Windows || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__) && !defined AMX_NODYNALOAD
  { /* local */
    int i;
    #if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
      root=getenv("AMXLIB");
    #endif
    hdr=(AMX_HEADER *)amx->base;
    numlibraries=NUMENTRIES(hdr,libraries,pubvars);
    for (i=0; i<numlibraries; i++) {
      lib=GETENTRY(hdr,libraries,i);
      libname[0]='\0';
      #if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
        if (root!=NULL && *root!='\0') {
          strcpy(libname,root);
          if (libname[strlen(libname)-1]!='/')
            strcat(libname,"/");
        } /* if */
      #endif
      strcat(libname,"amx");
      strcat(libname,GETENTRYNAME(hdr,lib));
      #if defined _Windows
        strcat(libname,".dll");
        #if defined __WIN32__
          hlib=LoadLibraryA(libname);
        #else
          hlib=LoadLibrary(libname);
          if (hlib<=HINSTANCE_ERROR)
            hlib=NULL;
        #endif
      #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
        strcat(libname,".so");
        hlib=dlopen(libname,RTLD_NOW);
      #endif
      if (hlib!=NULL) {
        /* a library that cannot be loaded or that does not have the required
         * initialization function is simply ignored
         */
        char funcname[sNAMEMAX+9]; /* +1 for '\0', +4 for 'amx_', +4 for 'Init' */
        strcpy(funcname,"amx_");
        strcat(funcname,GETENTRYNAME(hdr,lib));
        strcat(funcname,"Init");
        #if defined _Windows
          libinit=(AMX_ENTRY)GetProcAddress(hlib,funcname);
        #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
          libinit=(AMX_ENTRY)dlsym(hlib,funcname);
        #endif
        if (libinit!=NULL)
          libinit(amx);
      } /* if */
      lib->address=(ucell)(intptr_t)hlib;
    } /* for */
  } /* local */
  #endif

  return AMX_ERR_NONE;
}

#if defined AMX_JIT

  #define CODESIZE_JIT    8192  /* approximate size of the code for the JIT */

  #if defined __WIN32__   /* this also applies to Win32 "console" applications */

    #define ALIGN(addr)     (addr)

    #define PROT_READ       0x1         /* page can be read */
    #define PROT_WRITE      0x2         /* page can be written */
    #define PROT_EXEC       0x4         /* page can be executed */
    #define PROT_NONE       0x0         /* page can not be accessed */

    static int mprotect(void *addr, size_t len, int prot)
    {
      DWORD prev, p = 0;
      if ((prot & PROT_WRITE)!=0)
        p = PAGE_EXECUTE_READWRITE;
      else
        p |= PAGE_EXECUTE_READ;
      return !VirtualProtect(addr, len, p, &prev);
    }

  #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__

    /* Linux, BSD and OSX already have mprotect() */
    #define ALIGN(addr) (char *)(((long)addr + sysconf(_SC_PAGESIZE)-1) & ~(sysconf(_SC_PAGESIZE)-1))

  #else

    // TODO: Add cases for Mac OS/X and other operating systems

    /* DOS32 has no imposed limits on its segments */
    #define ALIGN(addr)     (addr)
    #define mprotect(addr, len, prot)   (0)

  #endif /* #if defined __WIN32 __ */

int AMXAPI amx_InitJIT(AMX *amx, void *reloc_table, void *native_code)
{
  int res;
  AMX_HEADER *hdr;

  if ((amx->flags & AMX_FLAG_JITC)==0)
    return AMX_ERR_INIT_JIT;    /* flag not set, this AMX is not prepared for JIT */
  if (hdr->file_version>MAX_FILE_VER_JIT)
    return AMX_ERR_VERSION;     /* JIT may not support the newest file version(s) */
  /* the JIT does not support overlays, but this is already checked in VerifyPcode()
   * the same goes for macro instructions: not supported in the JIT, but already
   * checked in VerifyPcode()
   */

  /* Patching SYSREQ(.N) opcodes to SYSREQ.(N)D cannot work in the JIT, because
   * the program would need to be re-JIT-compiled after patching a P-code
   * instruction. If this field is not zero, something went wrong in
   * VerifyPcode().
   */
  assert(amx->sysreq_d==0);

  if (mprotect(ALIGN(amx_jit_compile), CODESIZE_JIT, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    return AMX_ERR_INIT_JIT;

  /* copy the prefix */
  memcpy(native_code, amx->base, ((AMX_HEADER *)(amx->base))->cod);
  hdr = native_code;

  /* MP: added check for correct compilation */
  if ((res = amx_jit_compile(amx->base, reloc_table, native_code)) == 0) {
    /* update the required memory size (the previous value was a
     * conservative estimate, now we know the exact size)
     */
    amx->codesize = (hdr->dat + hdr->stp + sizeof(cell)) & ~(sizeof(cell)-1);
    /* The compiled code is relocatable, since only relative jumps are
     * used for destinations within the generated code, and absolute
     * addresses are only for jumps into the runtime, which is fixed
     * in memory.
     */
    /* set the new pointers */
    amx->base = (unsigned char*)native_code;
    amx->code = amx->base + (int)hdr->cod;
    amx->cip = hdr->cip;
  } /* if */

  return (res == 0) ? AMX_ERR_NONE : AMX_ERR_INIT_JIT;
}

#else /* #if defined AMX_JIT */

int AMXAPI amx_InitJIT(AMX *amx,void *compiled_program,void *reloc_table)
{
  (void)amx;
  (void)compiled_program;
  (void)reloc_table;
  return AMX_ERR_INIT_JIT;
}

#endif  /* #if defined AMX_JIT */

#endif  /* AMX_INIT */

#if defined AMX_CLEANUP
int AMXAPI amx_Cleanup(AMX *amx)
{
  #if (defined _Windows || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__) && !defined AMX_NODYNALOAD
    AMX_HEADER *hdr;
    int numlibraries,i;
    AMX_FUNCSTUB *lib;
    AMX_ENTRY libcleanup;
  #endif

  /* unload all extension modules */
  #if (defined _Windows || defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__) && !defined AMX_NODYNALOAD
    hdr=(AMX_HEADER *)amx->base;
    assert(hdr->magic==AMX_MAGIC);
    numlibraries=NUMENTRIES(hdr,libraries,pubvars);
    for (i=0; i<numlibraries; i++) {
      lib=GETENTRY(hdr,libraries,i);
      if (lib->address!=0) {
        char funcname[sNAMEMAX+12]; /* +1 for '\0', +4 for 'amx_', +7 for 'Cleanup' */
        strcpy(funcname,"amx_");
        strcat(funcname,GETENTRYNAME(hdr,lib));
        strcat(funcname,"Cleanup");
        #if defined _Windows
          libcleanup=(AMX_ENTRY)GetProcAddress((HINSTANCE)lib->address,funcname);
        #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
          libcleanup=(AMX_ENTRY)dlsym((void*)(intptr_t)lib->address,funcname);
        #endif
        if (libcleanup!=NULL)
          libcleanup(amx);
        #if defined _Windows
          FreeLibrary((HINSTANCE)lib->address);
        #elif defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
          dlclose((void*)(intptr_t)lib->address);
        #endif
      } /* if */
    } /* for */
  #else
    (void)amx;
  #endif
  return AMX_ERR_NONE;
}
#endif /* AMX_CLEANUP */

#if defined AMX_CLONE
int AMXAPI amx_Clone(AMX *amxClone, AMX *amxSource, void *data)
{
  AMX_HEADER *hdr;
  unsigned char _FAR *dataSource;

  if (amxSource==NULL)
    return AMX_ERR_FORMAT;
  if (amxClone==NULL)
    return AMX_ERR_PARAMS;
  if ((amxSource->flags & AMX_FLAG_INIT)==0)
    return AMX_ERR_INIT;
  hdr=(AMX_HEADER *)amxSource->base;
  if (hdr->magic!=AMX_MAGIC)
    return AMX_ERR_FORMAT;
  if (hdr->file_version>CUR_FILE_VERSION || hdr->amx_version<MIN_FILE_VERSION)
    return AMX_ERR_VERSION;

  /* set initial values */
  amxClone->base=amxSource->base;
  amxClone->code=amxSource->code;
  amxClone->codesize=amxSource->codesize;
  amxClone->hlw=hdr->hea - hdr->dat; /* stack and heap relative to data segment */
  amxClone->stp=hdr->stp - hdr->dat - sizeof(cell);
  amxClone->hea=amxClone->hlw;
  amxClone->stk=amxClone->stp;
  if (amxClone->callback==NULL)
    amxClone->callback=amxSource->callback;
  if (amxClone->debug==NULL)
    amxClone->debug=amxSource->debug;
  amxClone->flags=amxSource->flags;

  /* copy the data segment; the stack and the heap can be left uninitialized */
  assert(data!=NULL);
  amxClone->data=(unsigned char _FAR *)data;
  dataSource=(amxSource->data!=NULL) ? amxSource->data : amxSource->base+(int)hdr->dat;
  memcpy(amxClone->data,dataSource,(size_t)(hdr->hea-hdr->dat));

  /* Set a zero cell at the top of the stack, which functions
   * as a sentinel for strings.
   */
  * (cell *)(amxClone->data+(int)amxClone->stp) = 0;

  return AMX_ERR_NONE;
}
#endif /* AMX_CLONE */

#if defined AMX_MEMINFO
int AMXAPI amx_MemInfo(AMX *amx, long *codesize, long *datasize, long *stackheap)
{
  AMX_HEADER *hdr;

  if (amx==NULL)
    return AMX_ERR_FORMAT;
  hdr=(AMX_HEADER *)amx->base;
  if (hdr->magic!=AMX_MAGIC)
    return AMX_ERR_FORMAT;
  if (hdr->file_version>CUR_FILE_VERSION || hdr->amx_version<MIN_FILE_VERSION)
    return AMX_ERR_VERSION;

  if (codesize!=NULL)
    *codesize=amx->codesize;
  if (datasize!=NULL)
    *datasize=hdr->hea - hdr->dat;
  if (stackheap!=NULL)
    *stackheap=hdr->stp - hdr->hea;

  return AMX_ERR_NONE;
}
#endif /* AMX_MEMINFO */

#if defined AMX_NAMELENGTH
int AMXAPI amx_NameLength(AMX *amx, int *length)
{
  AMX_HEADER *hdr;
  uint16_t *namelength;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  namelength=(uint16_t*)(amx->base + (unsigned)hdr->nametable);
  *length=*namelength;
  return AMX_ERR_NONE;
}
#endif /* AMX_NAMELENGTH */

#if defined AMX_XXXNATIVES
int AMXAPI amx_NumNatives(AMX *amx, int *number)
{
  AMX_HEADER *hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->natives<=hdr->libraries);
  *number=NUMENTRIES(hdr,natives,libraries);
  return AMX_ERR_NONE;
}

int AMXAPI amx_GetNative(AMX *amx, int index, char *name)
{
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *func;

  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->natives<=hdr->libraries);
  if (index>=(cell)NUMENTRIES(hdr,natives,libraries))
    return AMX_ERR_INDEX;

  func=GETENTRY(hdr,natives,index);
  strcpy(name,GETENTRYNAME(hdr,func));
  return AMX_ERR_NONE;
}

int AMXAPI amx_FindNative(AMX *amx, const char *name, int *index)
{
  int idx,last;
  char pname[sNAMEMAX+1];

  amx_NumNatives(amx, &last);
  /* linear search, the natives table is not sorted alphabetically */
  for (idx=0; idx<last; idx++) {
    amx_GetNative(amx,idx,pname);
    if (strcmp(pname,name)==0) {
      *index=idx;
      return AMX_ERR_NONE;
    } /* if */
  } /* for */
  *index=INT_MAX;
  return AMX_ERR_NOTFOUND;
}
#endif /* AMX_XXXNATIVES */

#if defined AMX_XXXPUBLICS
int AMXAPI amx_NumPublics(AMX *amx, int *number)
{
  AMX_HEADER *hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->publics<=hdr->natives);
  *number=NUMENTRIES(hdr,publics,natives);
  return AMX_ERR_NONE;
}

int AMXAPI amx_GetPublic(AMX *amx, int index, char *name, ucell *address)
{
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *func;

  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->publics<=hdr->natives);
  if (index>=(cell)NUMENTRIES(hdr,publics,natives))
    return AMX_ERR_INDEX;

  func=GETENTRY(hdr,publics,index);
  if (name!=NULL)
    strcpy(name,GETENTRYNAME(hdr,func));
  if (address!=NULL)
    *address=func->address;
  return AMX_ERR_NONE;
}

int AMXAPI amx_FindPublic(AMX *amx, const char *name, int *index)
{
  int first,last,mid,result;
  char pname[sNAMEMAX+1];

  amx_NumPublics(amx, &last);
  last--;       /* last valid index is 1 less than the number of functions */
  first=0;
  /* binary search */
  while (first<=last) {
    mid=(first+last)/2;
    amx_GetPublic(amx,mid,pname,NULL);
    result=strcmp(pname,name);
    if (result>0) {
      last=mid-1;
    } else if (result<0) {
      first=mid+1;
    } else {
      *index=mid;
      return AMX_ERR_NONE;
    } /* if */
  } /* while */
  /* not found, set to an invalid index, so amx_Exec() on this index will fail
   * with an error
   */
  *index=INT_MAX;
  return AMX_ERR_NOTFOUND;
}
#endif /* AMX_XXXPUBLICS */

#if defined AMX_XXXPUBVARS
int AMXAPI amx_NumPubVars(AMX *amx, int *number)
{
  AMX_HEADER *hdr;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->pubvars<=hdr->tags);
  *number=NUMENTRIES(hdr,pubvars,tags);
  return AMX_ERR_NONE;
}

int AMXAPI amx_GetPubVar(AMX *amx, int index, char *name, cell **address)
{
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *var;
  unsigned char *data;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->pubvars<=hdr->tags);
  if (index>=(cell)NUMENTRIES(hdr,pubvars,tags))
    return AMX_ERR_INDEX;

  var=GETENTRY(hdr,pubvars,index);
  strcpy(name,GETENTRYNAME(hdr,var));
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;
  assert(address!=NULL);
  *address=(cell *)(data+(int)var->address);
  return AMX_ERR_NONE;
}

int AMXAPI amx_FindPubVar(AMX *amx, const char *name, cell **address)
{
  int first,last,mid,result;
  char pname[sNAMEMAX+1];

  amx_NumPubVars(amx,&last);
  last--;       /* last valid index is 1 less than the number of functions */
  first=0;
  /* binary search */
  while (first<=last) {
    mid=(first+last)/2;
    amx_GetPubVar(amx,mid,pname,address);
    result=strcmp(pname,name);
    if (result>0)
      last=mid-1;
    else if (result<0)
      first=mid+1;
    else
      return AMX_ERR_NONE;
  } /* while */
  /* not found */
  assert(address!=NULL);
  *address=NULL;
  return AMX_ERR_NOTFOUND;
}
#endif /* AMX_XXXPUBVARS */

#if defined AMX_XXXTAGS
int AMXAPI amx_NumTags(AMX *amx, int *number)
{
  AMX_HEADER *hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  if (hdr->file_version<5) {    /* the tagname table appeared in file format 5 */
    *number=0;
    return AMX_ERR_VERSION;
  } /* if */
  if (hdr->file_version<7) {    /* file version 7 introduced the name table */
    assert(hdr->tags<=hdr->cod);
    *number=NUMENTRIES(hdr,tags,cod);
  } else {
    assert(hdr->tags<=hdr->nametable);
    *number=NUMENTRIES(hdr,tags,nametable);
  } /* if */
  return AMX_ERR_NONE;
}

int AMXAPI amx_GetTag(AMX *amx, int index, char *tagname, cell *tag_id)
{
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *tag;

  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  if (hdr->file_version<5) {    /* the tagname table appeared in file format 5 */
    *tagname='\0';
    *tag_id=0;
    return AMX_ERR_VERSION;
  } /* if */

  if (hdr->file_version<7) {    /* file version 7 introduced the name table */
    assert(hdr->tags<=hdr->cod);
    if (index>=(cell)NUMENTRIES(hdr,tags,cod))
      return AMX_ERR_INDEX;
  } else {
    assert(hdr->tags<=hdr->nametable);
    if (index>=(cell)NUMENTRIES(hdr,tags,nametable))
      return AMX_ERR_INDEX;
  } /* if */

  tag=GETENTRY(hdr,tags,index);
  strcpy(tagname,GETENTRYNAME(hdr,tag));
  *tag_id=tag->address;

  return AMX_ERR_NONE;
}

int AMXAPI amx_FindTagId(AMX *amx, cell tag_id, char *tagname)
{
  int first,last,mid;
  cell mid_id;

  #if !defined NDEBUG
    /* verify that the tagname table is sorted on the tag_id */
    amx_NumTags(amx, &last);
    if (last>0) {
      cell cur_id;
      amx_GetTag(amx,0,tagname,&cur_id);
      for (first=1; first<last; first++) {
        amx_GetTag(amx,first,tagname,&mid_id);
        assert(cur_id<mid_id);
        cur_id=mid_id;
      } /* for */
    } /* if */
  #endif

  amx_NumTags(amx, &last);
  last--;       /* last valid index is 1 less than the number of functions */
  first=0;
  /* binary search */
  while (first<=last) {
    mid=(first+last)/2;
    amx_GetTag(amx,mid,tagname,&mid_id);
    if (mid_id>tag_id)
      last=mid-1;
    else if (mid_id<tag_id)
      first=mid+1;
    else
      return AMX_ERR_NONE;
  } /* while */
  /* not found */
  *tagname='\0';
  return AMX_ERR_NOTFOUND;
}
#endif /* AMX_XXXTAGS */

#if defined AMX_XXXUSERDATA
int AMXAPI amx_GetUserData(AMX *amx, long tag, void **ptr)
{
  int index;

  assert(amx!=NULL);
  assert(tag!=0);
  for (index=0; index<AMX_USERNUM && amx->usertags[index]!=tag; index++)
    /* nothing */;
  if (index>=AMX_USERNUM)
    return AMX_ERR_USERDATA;
  *ptr=amx->userdata[index];
  return AMX_ERR_NONE;
}

int AMXAPI amx_SetUserData(AMX *amx, long tag, void *ptr)
{
  int index;

  assert(amx!=NULL);
  assert(tag!=0);
  /* try to find existing tag */
  for (index=0; index<AMX_USERNUM && amx->usertags[index]!=tag; index++)
    /* nothing */;
  /* if not found, try to find empty tag */
  if (index>=AMX_USERNUM)
    for (index=0; index<AMX_USERNUM && amx->usertags[index]!=0; index++)
      /* nothing */;
  /* if still not found, quit with error */
  if (index>=AMX_USERNUM)
    return AMX_ERR_INDEX;
  /* set the tag and the value */
  amx->usertags[index]=tag;
  amx->userdata[index]=ptr;
  return AMX_ERR_NONE;
}
#endif /* AMX_XXXUSERDATA */

#if defined AMX_REGISTER
static AMX_NATIVE findfunction(char *name, const AMX_NATIVE_INFO *list, int number)
{
  int i;

  assert(list!=NULL);
  for (i=0; list[i].name!=NULL && (i<number || number==-1); i++)
    if (strcmp(name,list[i].name)==0)
      return list[i].func;
  return NULL;
}

int AMXAPI amx_Register(AMX *amx, const AMX_NATIVE_INFO *list, int number)
{
  AMX_FUNCSTUB *func;
  AMX_HEADER *hdr;
  int i,numnatives,err;
  AMX_NATIVE funcptr;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->natives<=hdr->libraries);
  numnatives=NUMENTRIES(hdr,natives,libraries);

  err=AMX_ERR_NONE;
  func=GETENTRY(hdr,natives,0);
  for (i=0; i<numnatives; i++) {
    if (func->address==0) {
      /* this function is not yet located */
      funcptr=(list!=NULL) ? findfunction(GETENTRYNAME(hdr,func),list,number) : NULL;
      if (funcptr!=NULL)
        func->address=(ucell)(intptr_t)funcptr;
      else
        err=AMX_ERR_NOTFOUND;
    } /* if */
    func=(AMX_FUNCSTUB*)((unsigned char*)func+hdr->defsize);
  } /* for */
  if (err==AMX_ERR_NONE)
    amx->flags|=AMX_FLAG_NTVREG;
  return err;
}
#endif /* AMX_REGISTER */

#if defined AMX_NATIVEINFO
AMX_NATIVE_INFO * AMXAPI amx_NativeInfo(const char *name, AMX_NATIVE func)
{
  static AMX_NATIVE_INFO n;
  n.name=name;
  n.func=func;
  return &n;
}
#endif /* AMX_NATIVEINFO */


#define STKMARGIN       ((cell)(16*sizeof(cell)))

#if defined AMX_PUSHXXX

int AMXAPI amx_Push(AMX *amx, cell value)
{
  AMX_HEADER *hdr;
  unsigned char *data;

  if (amx->hea+STKMARGIN>amx->stk)
    return AMX_ERR_STACKERR;
  hdr=(AMX_HEADER *)amx->base;
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;
  amx->stk-=sizeof(cell);
  amx->paramcount+=1;
  *(cell *)(data+(int)amx->stk)=value;
  return AMX_ERR_NONE;
}

int AMXAPI amx_PushAddress(AMX *amx, cell *address)
{
  AMX_HEADER *hdr;
  unsigned char *data;
  cell xaddr;

  /* reverse relocate the address */
  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;
  xaddr=(cell)((unsigned char*)address-data);
  if ((ucell)xaddr>=(ucell)amx->stp)
    return AMX_ERR_MEMACCESS;
  return amx_Push(amx,xaddr);
}

int AMXAPI amx_PushArray(AMX *amx, cell **address, const cell array[], int numcells)
{
  cell xaddr,*paddr;
  int err;

  assert(amx!=NULL);
  assert(array!=NULL);

  xaddr=amx->hea; /* save, before it is modified by amx_Allot() */
  err=amx_Allot(amx,numcells,&paddr);
  if (err==AMX_ERR_NONE) {
    memcpy(paddr,array,numcells*sizeof(cell));
    err=amx_Push(amx,xaddr);
  } /* if */
  if (address!=NULL)
    *address=paddr;
  return err;
}

int AMXAPI amx_PushString(AMX *amx, cell **address, const char *string, int pack, int use_wchar)
{
  cell xaddr,*paddr;
  int numcells,err;

  assert(amx!=NULL);
  assert(string!=NULL);

  #if defined AMX_ANSIONLY
    numcells=strlen(string) + 1;
  #else
    numcells= (int)(use_wchar ? wcslen((const wchar_t*)string) : strlen(string)) + 1;
  #endif
  if (pack)
    numcells=(numcells+sizeof(cell)-1)/sizeof(cell);
  xaddr=amx->hea; /* save, before it is modified by amx_Allot() */
  err=amx_Allot(amx,numcells,&paddr);
  if (err==AMX_ERR_NONE) {
    amx_SetString(paddr,string,pack,use_wchar,numcells);
    err=amx_Push(amx,xaddr);
  } /* if */
  if (address!=NULL)
    *address=paddr;
  return err;
}
#endif /* AMX_PUSHXXX */

#if defined AMX_EXEC

/* It is assumed that the abstract machine can simply access the memory area
 * for the global data and the stack. If this is not the case, you need to
 * define the macro sets _R() and _W(), for reading and writing to memory.
 */
#if !defined _R
  #define _R_DEFAULT            /* mark default memory access */
  #define _R(base,addr)         (* (cell *)((unsigned char*)(base)+(int)(addr)))
  #define _R8(base,addr)        (* (unsigned char *)((unsigned char*)(base)+(int)(addr)))
  #define _R16(base,addr)       (* (uint16_t *)((unsigned char*)(base)+(int)(addr)))
  #define _R32(base,addr)       (* (uint32_t *)((unsigned char*)(base)+(int)(addr)))
#endif
#if !defined _W
  #define _W_DEFAULT            /* mark default memory access */
  #define _W(base,addr,value)   ((*(cell *)((unsigned char*)(base)+(int)(addr)))=(cell)(value))
  #define _W8(base,addr,value)  ((*(unsigned char *)((unsigned char*)(base)+(int)(addr)))=(unsigned char)(value))
  #define _W16(base,addr,value) ((*(uint16_t *)((unsigned char*)(base)+(int)(addr)))=(uint16_t)(value))
  #define _W32(base,addr,value) ((*(uint32_t *)((unsigned char*)(base)+(int)(addr)))=(uint32_t)(value))
#endif

#if -8/3==-2 && 8/-3==-2
  #define TRUNC_SDIV    /* signed divisions are truncated on this platform */
#else
  #define IABS(a)       ((a)>=0 ? (a) : (-a))
#endif

/* The pseudo-instructions come from the code stream. Normally, these are just
 * accessed from memory. When the instructions must be fetched in some other
 * way, the definition below must be pre-defined.
 * N.B.:
 *   - reading from a code address should increment the instruction pointer
 *     (called "cip")
 *   - only cell-sized accesses occur in code memory
 */
#if !defined _RCODE
  #define _RCODE()      ( *cip++ )
#endif

#if !defined GETPARAM
  #define GETPARAM(v)   ( v=_RCODE() )   /* read a parameter from the opcode stream */
#endif
#if !defined SKIPPARAM
  #define SKIPPARAM(n)  ( cip=(cell *)cip+(n) ) /* for obsolete opcodes */
#endif

#define AMXPUSH(v)      ( amx->stk-=sizeof(cell), *(cell*)(data+amx->stk)=(v) )
#define ABORT(amx,v)    { (amx)->stk=reset_stk; (amx)->hea=reset_hea; return v; }


#if !defined AMX_ALTCORE
int amx_exec_list(AMX *amx,const cell **opcodelist,int *numopcodes)
{
  (void)amx;
  assert(opcodelist!=NULL);
  *opcodelist=NULL;
  assert(numopcodes!=NULL);
  *numopcodes=OP_NUM_OPCODES;
  return 0;
}
#endif

int AMXAPI amx_Exec(AMX *amx, cell *retval, int index)
{
  AMX_HEADER *hdr;
  AMX_FUNCSTUB *func;
  unsigned char *data;
  cell reset_stk,reset_hea;
  int i;
#if !defined AMX_ALTCORE
  cell pri,alt,stk,frm,hea;
  cell *cip,op,offs,val;
#endif

  assert(amx!=NULL);
  if ((amx->flags & AMX_FLAG_INIT)==0)
    return AMX_ERR_INIT;
  if (amx->callback==NULL)
    return AMX_ERR_CALLBACK;

  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);

  if ((amx->flags & AMX_FLAG_NTVREG)==0) {
    /* verify that all native functions have been registered (or do not
     * need registering)
     */
    int numnatives;
    assert(hdr->natives<=hdr->libraries);
    numnatives=NUMENTRIES(hdr,natives,libraries);
    func=GETENTRY(hdr,natives,0);
    for (i=0; i<numnatives && func->address!=0; i++)
      func=(AMX_FUNCSTUB*)((unsigned char*)func+hdr->defsize);
    if (i<numnatives)
      return AMX_ERR_NOTFOUND;
    amx->flags|=AMX_FLAG_NTVREG;  /* no need to check this again */
  } /* if */
  assert((amx->flags & AMX_FLAG_VERIFY)==0);

  /* set up the registers */
  assert(hdr!=NULL && hdr->magic==AMX_MAGIC);
  assert(amx->code!=NULL || hdr->overlays!=hdr->nametable);
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;
  reset_stk=amx->stk;
  reset_hea=amx->hea;
  amx->error=AMX_ERR_NONE;

  /* get the start address */
  if (index==AMX_EXEC_MAIN) {
    if (hdr->cip<0)
      return AMX_ERR_INDEX;
    amx->cip=hdr->cip;
    if (hdr->overlays!=hdr->nametable) {
      assert(hdr->overlays!=0);
      assert(amx->overlay!=NULL);
      amx->ovl_index=(int)hdr->cip;
      if ((i=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
        return i;
      amx->cip=0;
    } /* if */
  } else if (index==AMX_EXEC_CONT) {
    /* restore registers reset_stk & reset_hea */
    reset_stk=amx->reset_stk;
    reset_hea=amx->reset_hea;
    if (hdr->overlays!=hdr->nametable) {
      assert(hdr->overlays!=0);
      assert(amx->overlay!=NULL);
      if ((i=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
        return i;
    } /* if */
  } else if (index<0) {
    return AMX_ERR_INDEX;
  } else {
    if (index>=(int)NUMENTRIES(hdr,publics,natives))
      return AMX_ERR_INDEX;
    func=GETENTRY(hdr,publics,index);
    amx->cip=func->address;
    if (hdr->overlays!=hdr->nametable) {
      assert(hdr->overlays!=0);
      assert(amx->overlay!=NULL);
      amx->ovl_index=func->address;
      if ((i=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
        return i;
      amx->cip=0;
    } /* if */
  } /* if */
  /* check values just copied */
  if (amx->stk>amx->stp)
    return AMX_ERR_STACKLOW;
  if (amx->hea<amx->hlw)
    return AMX_ERR_HEAPLOW;
  assert(check_endian());

  /* sanity checks */
  assert_static(OP_XCHG==21);
  assert_static(OP_SMUL==42);
  assert_static(OP_MOVS==64);
  #if !defined AMX_NO_MACRO_INSTR
    assert_static(OP_LIDX==81);
    assert_static(OP_ZERO_PRI==102);
    assert_static(OP_LOAD2==120);
  #endif
  #if !defined AMX_NO_PACKED_OPC
    assert_static(OP_LOAD_P_PRI==124);
    assert_static(OP_ALIGN_P_PRI==141);
    assert_static(OP_BOUNDS_P==174);
  #endif
  #if PAWN_CELL_SIZE==16
    assert_static(sizeof(cell)==2);
  #elif PAWN_CELL_SIZE==32
    assert_static(sizeof(cell)==4);
  #elif PAWN_CELL_SIZE==64
    assert_static(sizeof(cell)==8);
  #else
    #error Unsupported cell size
  #endif

  if (index!=AMX_EXEC_CONT) {
    reset_stk+=amx->paramcount*sizeof(cell);
    AMXPUSH(amx->paramcount*sizeof(cell));
    amx->paramcount=0;          /* push the parameter count to the stack & reset */
    AMXPUSH(0);                 /* return address (for overlays: overlay 0, offset 0) */
  } /* if */
  /* check stack/heap before starting to run */
  if (amx->hea+STKMARGIN>amx->stk)
    return AMX_ERR_STACKERR;

#if defined AMX_ALTCORE

  /* start running either the ARM or 80x86 assembler abstract machine or the JIT */
  #if defined AMX_ASM && defined AMX_JIT
    if ((amx->flags & AMX_FLAG_JITC)!=0)
      i = amx_jit_run(amx,retval,data);
    else
      i = amx_exec_run(amx,retval,data);
  #elif defined AMX_JIT
    i = amx_jit_run(amx,retval,data);
  #else
    /* also for GNU GCC and Intel C/C++ versions */
    i = amx_exec_run(amx,retval,data);
  #endif
  if (i == AMX_ERR_SLEEP) {
    amx->reset_stk=reset_stk;
    amx->reset_hea=reset_hea;
  } else {
    /* remove parameters from the stack; do this the "hard" way, because
     * the assembler version has no internal knowledge of the local
     * variables, so any "clean" way would be a kludge anyway.
     */
    amx->stk=reset_stk;
    amx->hea=reset_hea;
  } /* if */
  return i;

#else

  #define CHKMARGIN()   if (hea+STKMARGIN>stk) return AMX_ERR_STACKERR
  #define CHKSTACK()    if (stk>amx->stp) return AMX_ERR_STACKLOW
  #define CHKHEAP()     if (hea<amx->hlw) return AMX_ERR_HEAPLOW

  /* PUSH() and POP() are defined in terms of the _R() and _W() macros */
  #define PUSH(v)       ( stk-=sizeof(cell), _W(data,stk,v) )
  #define POP(v)        ( v=_R(data,stk), stk+=sizeof(cell) )

  /* set up registers for ANSI-C core: pri, alt, frm, cip, hea, stk */
  pri=amx->pri;
  alt=amx->alt;
  frm=amx->frm;
  cip=(cell *)(amx->code+(int)amx->cip);
  hea=amx->hea;
  stk=amx->stk;

  /* start running */
  for ( ;; ) {
    op=_RCODE();
    switch (GETOPCODE(op)) {
    /* core instruction set */
    case OP_NOP:
      break;
    case OP_LOAD_PRI:
      GETPARAM(offs);
      pri=_R(data,offs);
      break;
    case OP_LOAD_ALT:
      GETPARAM(offs);
      alt=_R(data,offs);
      break;
    case OP_LOAD_S_PRI:
      GETPARAM(offs);
      pri=_R(data,frm+offs);
      break;
    case OP_LOAD_S_ALT:
      GETPARAM(offs);
      alt=_R(data,frm+offs);
      break;
    case OP_LREF_S_PRI:
      GETPARAM(offs);
      offs=_R(data,frm+offs);
      pri=_R(data,offs);
      break;
    case OP_LREF_S_ALT:
      GETPARAM(offs);
      offs=_R(data,frm+offs);
      alt=_R(data,offs);
      break;
    case OP_LOAD_I:
      /* verify address */
      if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      pri=_R(data,pri);
      break;
    case OP_LODB_I:
      GETPARAM(offs);
    __lodb_i:
      /* verify address */
      if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      switch ((int)offs) {
      case 1:
        pri=_R8(data,pri);
        break;
      case 2:
        pri=_R16(data,pri);
        break;
      case 4:
        pri=_R32(data,pri);
        break;
      } /* switch */
      break;
    case OP_CONST_PRI:
      GETPARAM(pri);
      break;
    case OP_CONST_ALT:
      GETPARAM(alt);
      break;
    case OP_ADDR_PRI:
      GETPARAM(pri);
      pri+=frm;
      break;
    case OP_ADDR_ALT:
      GETPARAM(alt);
      alt+=frm;
      break;
    case OP_STOR:
      GETPARAM(offs);
      _W(data,offs,pri);
      break;
    case OP_STOR_S:
      GETPARAM(offs);
      _W(data,frm+offs,pri);
      break;
    case OP_SREF_S:
      GETPARAM(offs);
      offs=_R(data,frm+offs);
      _W(data,offs,pri);
      break;
    case OP_STOR_I:
      /* verify address */
      if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      _W(data,alt,pri);
      break;
    case OP_STRB_I:
      GETPARAM(offs);
    __strb_i:
      /* verify address */
      if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      switch ((int)offs) {
      case 1:
        _W8(data,alt,pri);
        break;
      case 2:
        _W16(data,alt,pri);
        break;
      case 4:
        _W32(data,alt,pri);
        break;
      } /* switch */
      break;
    case OP_ALIGN_PRI:
      GETPARAM(offs);
      #if BYTE_ORDER==LITTLE_ENDIAN
        if ((size_t)offs<sizeof(cell))
          pri ^= sizeof(cell)-offs;
      #endif
      break;
    case OP_LCTRL:
      GETPARAM(offs);
      switch ((int)offs) {
      case 0:
        pri=hdr->cod;
        break;
      case 1:
        pri=hdr->dat;
        break;
      case 2:
        pri=hea;
        break;
      case 3:
        pri=amx->stp;
        break;
      case 4:
        pri=stk;
        break;
      case 5:
        pri=frm;
        break;
      case 6:
        pri=(cell)((unsigned char *)cip-amx->code);
        break;
      } /* switch */
      break;
    case OP_SCTRL:
      GETPARAM(offs);
      switch ((int)offs) {
      case 0:
      case 1:
      case 3:
        /* cannot change these parameters */
        break;
      case 2:
        hea=pri;
        break;
      case 4:
        stk=pri;
        break;
      case 5:
        frm=pri;
        break;
      case 6:
        cip=(cell *)(amx->code + (int)pri);
        break;
      } /* switch */
      break;
    case OP_XCHG:
      offs=pri;         /* offs is a temporary variable */
      pri=alt;
      alt=offs;
      break;
    case OP_PUSH_PRI:
      PUSH(pri);
      break;
    case OP_PUSH_ALT:
      PUSH(alt);
      break;
    case OP_PUSHR_PRI:
      PUSH((intptr_t)data+pri);
      break;
    case OP_POP_PRI:
      POP(pri);
      break;
    case OP_POP_ALT:
      POP(alt);
      break;
    case OP_PICK:
      GETPARAM(offs);
      pri=_R(data,stk+offs);
      break;
    case OP_STACK:
      GETPARAM(offs);
      alt=stk;
      stk+=offs;
      CHKMARGIN();
      CHKSTACK();
      break;
    case OP_HEAP:
      GETPARAM(offs);
      alt=hea;
      hea+=offs;
      CHKMARGIN();
      CHKHEAP();
      break;
    case OP_PROC:
      PUSH(frm);
      frm=stk;
      CHKMARGIN();
      break;
    case OP_RET:
      POP(frm);
      POP(offs);
      /* verify the return address */
      if ((long)offs>=amx->codesize)
        ABORT(amx,AMX_ERR_MEMACCESS);
      cip=(cell *)(amx->code+(int)offs);
      break;
    case OP_RETN:
      POP(frm);
      POP(offs);
      /* verify the return address */
      if ((long)offs>=amx->codesize)
        ABORT(amx,AMX_ERR_MEMACCESS);
      cip=(cell *)(amx->code+(int)offs);
      stk+=_R(data,stk)+sizeof(cell);   /* remove parameters from the stack */
      break;
    case OP_CALL:
      PUSH(((unsigned char *)cip-amx->code)+sizeof(cell));/* skip address */
      cip=JUMPREL(cip);                 /* jump to the address */
      break;
    case OP_JUMP:
      /* since the GETPARAM() macro modifies cip, you cannot
       * do GETPARAM(cip) directly */
      cip=JUMPREL(cip);
      break;
    case OP_JZER:
      if (pri==0)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JNZ:
      if (pri!=0)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_SHL:
      pri<<=alt;
      break;
    case OP_SHR:
      pri=(ucell)pri >> (int)alt;
      break;
    case OP_SSHR:
      pri>>=alt;
      break;
    case OP_SHL_C_PRI:
      GETPARAM(offs);
      pri<<=offs;
      break;
    case OP_SHL_C_ALT:
      GETPARAM(offs);
      alt<<=offs;
      break;
    case OP_SMUL:
      pri*=alt;
      break;
    case OP_SDIV:
      if (pri==0)
        ABORT(amx,AMX_ERR_DIVIDE);
      /* use floored division and matching remainder */
      offs=pri;
      #if defined TRUNC_SDIV
        pri=alt/offs;
        alt=alt%offs;
      #else
        val=alt;                /* portable routine for truncated division */
        pri=IABS(alt)/IABS(offs);
        if ((cell)(val ^ offs)<0)
          pri=-pri;
        alt=val-pri*offs;       /* calculate the matching remainder */
      #endif
      /* now "fiddle" with the values to get floored division */
      if (alt!=0 && (cell)(alt ^ offs)<0) {
        pri--;
        alt+=offs;
      } /* if */
      break;
    case OP_ADD:
      pri+=alt;
      break;
    case OP_SUB:
      pri=alt-pri;
      break;
    case OP_AND:
      pri&=alt;
      break;
    case OP_OR:
      pri|=alt;
      break;
    case OP_XOR:
      pri^=alt;
      break;
    case OP_NOT:
      pri=!pri;
      break;
    case OP_NEG:
      pri=-pri;
      break;
    case OP_INVERT:
      pri=~pri;
      break;
    case OP_EQ:
      pri= pri==alt ? 1 : 0;
      break;
    case OP_NEQ:
      pri= pri!=alt ? 1 : 0;
      break;
    case OP_SLESS:
      pri= pri<alt ? 1 : 0;
      break;
    case OP_SLEQ:
      pri= pri<=alt ? 1 : 0;
      break;
    case OP_SGRTR:
      pri= pri>alt ? 1 : 0;
      break;
    case OP_SGEQ:
      pri= pri>=alt ? 1 : 0;
      break;
    case OP_INC_PRI:
      pri++;
      break;
    case OP_INC_ALT:
      alt++;
      break;
    case OP_INC_I:
      #if defined _R_DEFAULT
        *(cell *)(data+(int)pri) += 1;
      #else
        val=_R(data,pri);
        _W(data,pri,val+1);
      #endif
      break;
    case OP_DEC_PRI:
      pri--;
      break;
    case OP_DEC_ALT:
      alt--;
      break;
    case OP_DEC_I:
      #if defined _R_DEFAULT
        *(cell *)(data+(int)pri) -= 1;
      #else
        val=_R(data,pri);
        _W(data,pri,val-1);
      #endif
      break;
    case OP_MOVS:
      GETPARAM(offs);
    __movs:
      /* verify top & bottom memory addresses, for both source and destination
       * addresses
       */
      if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if ((pri+offs)>hea && (pri+offs)<stk || (ucell)(pri+offs)>(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if ((alt+offs)>hea && (alt+offs)<stk || (ucell)(alt+offs)>(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      #if defined _R_DEFAULT
        memcpy(data+(int)alt, data+(int)pri, (int)offs);
      #else
        for (i=0; i+4<offs; i+=4) {
          val=_R32(data,pri+i);
          _W32(data,alt+i,val);
        } /* for */
        for ( ; i<offs; i++) {
          val=_R8(data,pri+i);
          _W8(data,alt+i,val);
        } /* for */
      #endif
      break;
    case OP_CMPS:
      GETPARAM(offs);
    __cmps:
      /* verify top & bottom memory addresses, for both source and destination
       * addresses
       */
      if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if ((pri+offs)>hea && (pri+offs)<stk || (ucell)(pri+offs)>(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if ((alt+offs)>hea && (alt+offs)<stk || (ucell)(alt+offs)>(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      #if defined _R_DEFAULT
        pri=memcmp(data+(int)alt, data+(int)pri, (int)offs);
      #else
        pri=0;
        for (i=0; i+4<offs && pri==0; i+=4)
          pri=_R32(data,alt+i)-_R32(data,pri+i);
        for ( ; i<offs && pri==0; i++)
          pri=_R8(data,alt+i)-_R8(data,pri+i);
      #endif
      break;
    case OP_FILL:
      GETPARAM(offs);
    __fill:
      /* verify top & bottom memory addresses (destination only) */
      if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      if ((alt+offs)>hea && (alt+offs)<stk || (ucell)(alt+offs)>(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      for (i=(int)alt; (size_t)offs>=sizeof(cell); i+=sizeof(cell), offs-=sizeof(cell))
        _W32(data,i,pri);
      break;
    case OP_HALT:
      GETPARAM(offs);
    __halt:
      if (retval!=NULL)
        *retval=pri;
      /* store complete status (stk and hea are already set in the ABORT macro) */
      amx->frm=frm;
      amx->pri=pri;
      amx->alt=alt;
      amx->cip=(cell)((unsigned char*)cip-amx->code);
      if (offs==AMX_ERR_SLEEP) {
        amx->stk=stk;
        amx->hea=hea;
        amx->reset_stk=reset_stk;
        amx->reset_hea=reset_hea;
        return (int)offs;
      } /* if */
      ABORT(amx,(int)offs);
    case OP_BOUNDS:
      GETPARAM(offs);
      if ((ucell)pri>(ucell)offs) {
        amx->cip=(cell)((unsigned char *)cip-amx->code);
        ABORT(amx,AMX_ERR_BOUNDS);
      } /* if */
      break;
    case OP_SYSREQ:
      GETPARAM(offs);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      i=amx->callback(amx,offs,&pri,(cell *)(data+(int)stk));
      if (i!=AMX_ERR_NONE) {
        if (i==AMX_ERR_SLEEP) {
          amx->pri=pri;
          amx->alt=alt;
          amx->reset_stk=reset_stk;
          amx->reset_hea=reset_hea;
          return i;
        } /* if */
        ABORT(amx,i);
      } /* if */
      break;
    case OP_SWITCH: {
      cell *cptr=JUMPREL(cip)+1;/* +1, to skip the "casetbl" opcode */
      assert(*JUMPREL(cip)==OP_CASETBL);
      cip=JUMPREL(cptr+1);      /* preset to "none-matched" case */
      i=(int)*cptr;             /* number of records in the case table */
      for (cptr+=2; i>0 && *cptr!=pri; i--,cptr+=2)
        /* nothing */;
      if (i>0)
        cip=JUMPREL(cptr+1);    /* case found */
      break;
    } /* case */
    case OP_SWAP_PRI:
      offs=_R(data,stk);
      _W32(data,stk,pri);
      pri=offs;
      break;
    case OP_SWAP_ALT:
      offs=_R(data,stk);
      _W32(data,stk,alt);
      alt=offs;
      break;
    case OP_BREAK:
      assert((amx->flags & AMX_FLAG_VERIFY)==0);
      if (amx->debug!=NULL) {
        /* store status */
        amx->frm=frm;
        amx->stk=stk;
        amx->hea=hea;
        amx->cip=(cell)((unsigned char*)cip-amx->code);
        i=amx->debug(amx);
        if (i!=AMX_ERR_NONE) {
          if (i==AMX_ERR_SLEEP) {
            amx->pri=pri;
            amx->alt=alt;
            amx->reset_stk=reset_stk;
            amx->reset_hea=reset_hea;
            return i;
          } /* if */
          ABORT(amx,i);
        } /* if */
      } /* if */
      break;
#if !defined AMX_DONT_RELOCATE
    case OP_SYSREQ_D:    /* see SYSREQ */
      GETPARAM(offs);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      pri=((AMX_NATIVE)(intptr_t)offs)(amx,(cell*)(data+(int)stk));
      if (amx->error!=AMX_ERR_NONE) {
        if (amx->error==AMX_ERR_SLEEP) {
          amx->pri=pri;
          amx->alt=alt;
          amx->reset_stk=reset_stk;
          amx->reset_hea=reset_hea;
          return AMX_ERR_SLEEP;
        } /* if */
        ABORT(amx,amx->error);
      } /* if */
      break;
#endif
#if !defined AMX_NO_MACRO_INSTR && !defined AMX_DONT_RELOCATE
    case OP_SYSREQ_ND:    /* see SYSREQ_N */
      GETPARAM(offs);
      GETPARAM(val);
      PUSH(val);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      pri=((AMX_NATIVE)(intptr_t)offs)(amx,(cell*)(data+(int)stk));
      stk+=val+4;
      if (amx->error!=AMX_ERR_NONE) {
        if (amx->error==AMX_ERR_SLEEP) {
          amx->pri=pri;
          amx->alt=alt;
          amx->stk=stk;
          amx->reset_stk=reset_stk;
          amx->reset_hea=reset_hea;
          return AMX_ERR_SLEEP;
        } /* if */
        ABORT(amx,amx->error);
      } /* if */
      break;
#endif

    /* overlay instructions */
#if !defined AMX_NO_OVERLAY
    case OP_CALL_OVL:
      offs=(cell)((unsigned char *)cip-amx->code+sizeof(cell)); /* skip address */
      assert(offs>=0 && offs<(1<<(sizeof(cell)*4)));
      PUSH((offs<<(sizeof(cell)*4)) | amx->ovl_index);
      amx->ovl_index=(int)*cip;
      assert(amx->overlay!=NULL);
      if ((i=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
        ABORT(amx,i);
      cip=(cell*)amx->code;
      break;
    case OP_RETN_OVL:
      assert(amx->overlay!=NULL);
      POP(frm);
      POP(offs);
      amx->ovl_index=offs & (((ucell)~0)>>4*sizeof(cell));
      offs=(ucell)offs >> (sizeof(cell)*4);
      /* verify the index */
      stk+=_R(data,stk)+sizeof(cell);   /* remove parameters from the stack */
      i=amx->overlay(amx,amx->ovl_index); /* reload overlay */
      if (i!=AMX_ERR_NONE || (long)offs>=amx->codesize)
        ABORT(amx,AMX_ERR_MEMACCESS);
      cip=(cell *)(amx->code+(int)offs);
      break;
    case OP_SWITCH_OVL: {
      cell *cptr=JUMPREL(cip)+1;  /* +1, to skip the "icasetbl" opcode */
      assert(*JUMPREL(cip)==OP_CASETBL_OVL);
      amx->ovl_index=*(cptr+1);   /* preset to "none-matched" case */
      i=(int)*cptr;               /* number of records in the case table */
      for (cptr+=2; i>0 && *cptr!=pri; i--,cptr+=2)
        /* nothing */;
      if (i>0)
        amx->ovl_index=*(cptr+1); /* case found */
      assert(amx->overlay!=NULL);
      if ((i=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
        ABORT(amx,i);
      cip=(cell*)amx->code;
      break;
    } /* case */
#endif

    /* supplemental and macro instructions */
#if !defined AMX_NO_MACRO_INSTR
    case OP_LIDX:
      offs=pri*sizeof(cell)+alt;
      /* verify address */
      if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      pri=_R(data,offs);
      break;
    case OP_LIDX_B:
      GETPARAM(offs);
      offs=(pri << (int)offs)+alt;
      /* verify address */
      if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      pri=_R(data,offs);
      break;
    case OP_IDXADDR:
      pri=pri*sizeof(cell)+alt;
      break;
    case OP_IDXADDR_B:
      GETPARAM(offs);
      pri=(pri << (int)offs)+alt;
      break;
    case OP_PUSH_C:
      GETPARAM(offs);
      PUSH(offs);
      break;
    case OP_PUSH:
      GETPARAM(offs);
      PUSH(_R(data,offs));
      break;
    case OP_PUSH_S:
      GETPARAM(offs);
      PUSH(_R(data,frm+offs));
      break;
    case OP_PUSH_ADR:
      GETPARAM(offs);
      PUSH(frm+offs);
      break;
    case OP_PUSHR_C:
      GETPARAM(offs);
      PUSH((intptr_t)data+offs);
      break;
    case OP_PUSHR_S:
      GETPARAM(offs);
      PUSH((intptr_t)data+_R(data,frm+offs));
      break;
    case OP_PUSHR_ADR:
      GETPARAM(offs);
      PUSH((intptr_t)data+frm+offs);
      break;
    case OP_JEQ:
      if (pri==alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JNEQ:
      if (pri!=alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JSLESS:
      if (pri<alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JSLEQ:
      if (pri<=alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JSGRTR:
      if (pri>alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_JSGEQ:
      if (pri>=alt)
        cip=JUMPREL(cip);
      else
        SKIPPARAM(1);
      break;
    case OP_SDIV_INV:
      if (alt==0)
        ABORT(amx,AMX_ERR_DIVIDE);
      /* use floored division and matching remainder */
      offs=alt;
      #if defined TRUNC_SDIV
        pri=pri/offs;
        alt=pri%offs;
      #else
        val=pri;                /* portable routine for truncated division */
        pri=IABS(pri)/IABS(offs);
        if ((cell)(val ^ offs)<0)
          pri=-pri;
        alt=val-pri*offs;       /* calculate the matching remainder */
      #endif
      /* now "fiddle" with the values to get floored division */
      if (alt!=0 && (cell)(alt ^ offs)<0) {
        pri--;
        alt+=offs;
      } /* if */
      break;
    case OP_SUB_INV:
      pri-=alt;
      break;
    case OP_ADD_C:
      GETPARAM(offs);
      pri+=offs;
      break;
    case OP_SMUL_C:
      GETPARAM(offs);
      pri*=offs;
      break;
    case OP_ZERO_PRI:
      pri=0;
      break;
    case OP_ZERO_ALT:
      alt=0;
      break;
    case OP_ZERO:
      GETPARAM(offs);
      _W(data,offs,0);
      break;
    case OP_ZERO_S:
      GETPARAM(offs);
      _W(data,frm+offs,0);
      break;
    case OP_EQ_C_PRI:
      GETPARAM(offs);
      pri= pri==offs ? 1 : 0;
      break;
    case OP_EQ_C_ALT:
      GETPARAM(offs);
      pri= alt==offs ? 1 : 0;
      break;
    case OP_INC:
      GETPARAM(offs);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)offs) += 1;
      #else
        val=_R(data,offs);
        _W(data,offs,val+1);
      #endif
      break;
    case OP_INC_S:
      GETPARAM(offs);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)(frm+offs)) += 1;
      #else
        val=_R(data,frm+offs);
        _W(data,frm+offs,val+1);
      #endif
      break;
    case OP_DEC:
      GETPARAM(offs);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)offs) -= 1;
      #else
        val=_R(data,offs);
        _W(data,offs,val-1);
      #endif
      break;
    case OP_DEC_S:
      GETPARAM(offs);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)(frm+offs)) -= 1;
      #else
        val=_R(data,frm+offs);
        _W(data,frm+offs,val-1);
      #endif
      break;
    case OP_SYSREQ_N:
      GETPARAM(offs);
      GETPARAM(val);
      PUSH(val);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      i=amx->callback(amx,offs,&pri,(cell *)(data+(int)stk));
      stk+=val+4;
      if (i!=AMX_ERR_NONE) {
        if (i==AMX_ERR_SLEEP) {
          amx->pri=pri;
          amx->alt=alt;
          amx->stk=stk;
          amx->reset_stk=reset_stk;
          amx->reset_hea=reset_hea;
          return i;
        } /* if */
        ABORT(amx,i);
      } /* if */
      break;
    case OP_PUSHM_C:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH(offs);
      } /* while */
      break;
    case OP_PUSHM:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH(_R(data,offs));
      } /* while */
      break;
    case OP_PUSHM_S:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH(_R(data,frm+offs));
      } /* while */
      break;
    case OP_PUSHM_ADR:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH(frm+offs);
      } /* while */
      break;
    case OP_PUSHRM_C:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+offs);
      } /* while */
      break;
    case OP_PUSHRM_S:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+_R(data,frm+offs));
      } /* while */
      break;
    case OP_PUSHRM_ADR:
      GETPARAM(val);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+frm+offs);
      } /* while */
      break;
    case OP_LOAD2:
      GETPARAM(offs);
      pri=_R(data,offs);
      GETPARAM(offs);
      alt=_R(data,offs);
      break;
    case OP_LOAD2_S:
      GETPARAM(offs);
      pri=_R(data,frm+offs);
      GETPARAM(offs);
      alt=_R(data,frm+offs);
      break;
    case OP_CONST:
      GETPARAM(offs);
      GETPARAM(val);
      _W32(data,offs,val);
      break;
    case OP_CONST_S:
      GETPARAM(offs);
      GETPARAM(val);
      _W32(data,frm+offs,val);
      break;
#endif  /* AMX_NO_MACRO_INSTR */

#if !defined AMX_NO_PACKED_OPC
    case OP_LOAD_P_PRI:
      GETPARAM_P(offs,op);
      pri=_R(data,offs);
      break;
    case OP_LOAD_P_ALT:
      GETPARAM_P(offs,op);
      alt=_R(data,offs);
      break;
    case OP_LOAD_P_S_PRI:
      GETPARAM_P(offs,op);
      pri=_R(data,frm+offs);
      break;
    case OP_LOAD_P_S_ALT:
      GETPARAM_P(offs,op);
      alt=_R(data,frm+offs);
      break;
    case OP_LREF_P_S_PRI:
      GETPARAM_P(offs,op);
      offs=_R(data,frm+offs);
      pri=_R(data,offs);
      break;
    case OP_LREF_P_S_ALT:
      GETPARAM_P(offs,op);
      offs=_R(data,frm+offs);
      alt=_R(data,offs);
      break;
    case OP_LODB_P_I:
      GETPARAM_P(offs,op);
      goto __lodb_i;
    case OP_CONST_P_PRI:
      GETPARAM_P(pri,op);
      break;
    case OP_CONST_P_ALT:
      GETPARAM_P(alt,op);
      break;
    case OP_ADDR_P_PRI:
      GETPARAM_P(pri,op);
      pri+=frm;
      break;
    case OP_ADDR_P_ALT:
      GETPARAM_P(alt,op);
      alt+=frm;
      break;
    case OP_STOR_P:
      GETPARAM_P(offs,op);
      _W(data,offs,pri);
      break;
    case OP_STOR_P_S:
      GETPARAM_P(offs,op);
      _W(data,frm+offs,pri);
      break;
    case OP_SREF_P_S:
      GETPARAM_P(offs,op);
      offs=_R(data,frm+offs);
      _W(data,offs,pri);
      break;
    case OP_STRB_P_I:
      GETPARAM_P(offs,op);
      goto __strb_i;
    case OP_LIDX_P_B:
      GETPARAM_P(offs,op);
      offs=(pri << (int)offs)+alt;
      /* verify address */
      if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
        ABORT(amx,AMX_ERR_MEMACCESS);
      pri=_R(data,offs);
      break;
    case OP_IDXADDR_P_B:
      GETPARAM_P(offs,op);
      pri=(pri << (int)offs)+alt;
      break;
    case OP_ALIGN_P_PRI:
      GETPARAM_P(offs,op);
      #if BYTE_ORDER==LITTLE_ENDIAN
        if ((size_t)offs<sizeof(cell))
          pri ^= sizeof(cell)-offs;
      #endif
      break;
    case OP_PUSH_P_C:
      GETPARAM_P(offs,op);
      PUSH(offs);
      break;
    case OP_PUSH_P:
      GETPARAM_P(offs,op);
      PUSH(_R(data,offs));
      break;
    case OP_PUSH_P_S:
      GETPARAM_P(offs,op);
      PUSH(_R(data,frm+offs));
      break;
    case OP_PUSH_P_ADR:
      GETPARAM_P(offs,op);
      PUSH(frm+offs);
      break;
    case OP_PUSHR_P_C:
      GETPARAM_P(offs,op);
      PUSH((intptr_t)data+offs);
      break;
    case OP_PUSHR_P_S:
      GETPARAM_P(offs,op);
      PUSH((intptr_t)data+_R(data,frm+offs));
      break;
    case OP_PUSHR_P_ADR:
      GETPARAM_P(offs,op);
      PUSH((intptr_t)data+frm+offs);
      break;
    case OP_PUSHM_P:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH(_R(data,offs));
      } /* while */
      break;
    case OP_PUSHM_P_S:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH(_R(data,frm+offs));
      } /* while */
      break;
    case OP_PUSHM_P_C:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH(offs);
      } /* while */
      break;
    case OP_PUSHM_P_ADR:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH(frm+offs);
      } /* while */
      break;
    case OP_PUSHRM_P_C:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+offs);
      } /* while */
      break;
    case OP_PUSHRM_P_S:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+_R(data,frm+offs));
      } /* while */
      break;
    case OP_PUSHRM_P_ADR:
      GETPARAM_P(val,op);
      while (val--) {
        GETPARAM(offs);
        PUSH((intptr_t)data+frm+offs);
      } /* while */
      break;
    case OP_STACK_P:
      GETPARAM_P(offs,op);
      alt=stk;
      stk+=offs;
      CHKMARGIN();
      CHKSTACK();
      break;
    case OP_HEAP_P:
      GETPARAM_P(offs,op);
      alt=hea;
      hea+=offs;
      CHKMARGIN();
      CHKHEAP();
      break;
    case OP_SHL_P_C_PRI:
      GETPARAM_P(offs,op);
      pri<<=offs;
      break;
    case OP_SHL_P_C_ALT:
      GETPARAM_P(offs,op);
      alt<<=offs;
      break;
    case OP_ADD_P_C:
      GETPARAM_P(offs,op);
      pri+=offs;
      break;
    case OP_SMUL_P_C:
      GETPARAM_P(offs,op);
      pri*=offs;
      break;
    case OP_ZERO_P:
      GETPARAM_P(offs,op);
      _W(data,offs,0);
      break;
    case OP_ZERO_P_S:
      GETPARAM_P(offs,op);
      _W(data,frm+offs,0);
      break;
    case OP_EQ_P_C_PRI:
      GETPARAM_P(offs,op);
      pri= pri==offs ? 1 : 0;
      break;
    case OP_EQ_P_C_ALT:
      GETPARAM_P(offs,op);
      pri= alt==offs ? 1 : 0;
      break;
    case OP_INC_P:
      GETPARAM_P(offs,op);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)offs) += 1;
      #else
        val=_R(data,offs);
        _W(data,offs,val+1);
      #endif
      break;
    case OP_INC_P_S:
      GETPARAM_P(offs,op);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)(frm+offs)) += 1;
      #else
        val=_R(data,frm+offs);
        _W(data,frm+offs,val+1);
      #endif
      break;
    case OP_DEC_P:
      GETPARAM_P(offs,op);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)offs) -= 1;
      #else
        val=_R(data,offs);
        _W(data,offs,val-1);
      #endif
      break;
    case OP_DEC_P_S:
      GETPARAM_P(offs,op);
      #if defined _R_DEFAULT
        *(cell *)(data+(int)(frm+offs)) -= 1;
      #else
        val=_R(data,frm+offs);
        _W(data,frm+offs,val-1);
      #endif
      break;
    case OP_MOVS_P:
      GETPARAM_P(offs,op);
      goto __movs;
    case OP_CMPS_P:
      GETPARAM_P(offs,op);
      goto __cmps;
    case OP_FILL_P:
      GETPARAM_P(offs,op);
      goto __fill;
    case OP_HALT_P:
      GETPARAM_P(offs,op);
      goto __halt;
    case OP_BOUNDS_P:
      GETPARAM_P(offs,op);
      if ((ucell)pri>(ucell)offs) {
        amx->cip=(cell)((unsigned char *)cip-amx->code);
        ABORT(amx,AMX_ERR_BOUNDS);
      } /* if */
      break;
#endif /* AMX_NO_PACKED_OPC */
    default:
      assert(0);  /* invalid instructions should already have been caught in VerifyPcode() */
      ABORT(amx,AMX_ERR_INVINSTR);
    } /* switch */
  } /* for */
#endif /* AMX_ALTCORE */
}

#endif /* AMX_EXEC */

#if defined AMX_SETCALLBACK
int AMXAPI amx_SetCallback(AMX *amx,AMX_CALLBACK callback)
{
  assert(amx!=NULL);
  assert(callback!=NULL);
  amx->callback=callback;
  return AMX_ERR_NONE;
}
#endif /* AMX_SETCALLBACK */

#if defined AMX_SETDEBUGHOOK
int AMXAPI amx_SetDebugHook(AMX *amx,AMX_DEBUG debug)
{
  assert(amx!=NULL);
  amx->debug=debug;
  return AMX_ERR_NONE;
}
#endif /* AMX_SETDEBUGHOOK */

#if defined AMX_RAISEERROR
int AMXAPI amx_RaiseError(AMX *amx, int error)
{
  assert(error>0);
  amx->error=error;
  return AMX_ERR_NONE;
}
#endif /* AMX_RAISEERROR */

#if defined AMX_ALLOT || defined AMX_PUSHXXX
int AMXAPI amx_Allot(AMX *amx,int cells,cell **address)
{
  AMX_HEADER *hdr;
  unsigned char *data;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;

  if (amx->stk - amx->hea - cells*sizeof(cell) < STKMARGIN)
    return AMX_ERR_MEMORY;
  if (address!=NULL)
    *address=(cell *)(data+(int)amx->hea);
  amx->hea+=cells*sizeof(cell);
  return AMX_ERR_NONE;
}

int AMXAPI amx_Release(AMX *amx,cell *address)
{
  AMX_HEADER *hdr;
  unsigned char *data;
  cell amx_addr;

  assert(amx!=NULL);
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr!=NULL);
  assert(hdr->magic==AMX_MAGIC);
  data=(amx->data!=NULL) ? amx->data : amx->base+(int)hdr->dat;
  amx_addr=(cell)((unsigned char*)address-data);
  if (amx->hea>amx_addr)
    amx->hea=amx_addr;
  return AMX_ERR_NONE;
}
#endif /* AMX_ALLOT || AMX_PUSHXXX */

#if defined AMX_XXXSTRING || defined AMX_UTF8XXX

#define CHARBITS        (8*sizeof(char))
#if PAWN_CELL_SIZE==16
  #define CHARMASK      (0xffffu << 8*(2-sizeof(char)))
#elif PAWN_CELL_SIZE==32
  #define CHARMASK      (0xffffffffuL << 8*(4-sizeof(char)))
#elif PAWN_CELL_SIZE==64
  #define CHARMASK      (0xffffffffffffffffuLL << 8*(8-sizeof(char)))
#else
  #error Unsupported cell size
#endif

int AMXAPI amx_StrLen(const cell *cstr, int *length)
{
  int len;
  #if BYTE_ORDER==LITTLE_ENDIAN
    cell c;
  #endif

  assert(length!=NULL);
  if (cstr==NULL) {
    *length=0;
    return AMX_ERR_PARAMS;
  } /* if */

  if ((ucell)*cstr>UNPACKEDMAX) {
    /* packed string */
    assert_static(sizeof(char)==1);
    len=(int)strlen((char *)cstr);      /* find '\0' */
    assert(check_endian());
    #if BYTE_ORDER==LITTLE_ENDIAN
      /* on Little Endian machines, toggle the last bytes */
      c=cstr[len/sizeof(cell)];         /* get last cell */
      len=len - len % sizeof(cell);     /* len = multiple of "cell" bytes */
      while ((c & CHARMASK)!=0) {
        len++;
        c <<= 8*sizeof(char);
      } /* if */
    #endif
  } else {
    for (len=0; cstr[len]!=0; len++)
      /* nothing */;
  } /* if */
  *length = len;
  return AMX_ERR_NONE;
}
#endif

#if defined AMX_XXXSTRING || defined AMX_PUSHXXX
int AMXAPI amx_SetString(cell *dest,const char *source,int pack,int use_wchar,size_t size)
{                 /* the memory blocks should not overlap */
  int i;
  size_t len;

  assert_static(UNLIMITED>0);
  #if defined AMX_ANSIONLY
    (void)use_wchar;
    len=strlen(source);
  #else
    len= use_wchar ? wcslen((const wchar_t*)source) : strlen(source);
  #endif
  if (pack) {
    /* create a packed string */
    if (size<UNLIMITED/sizeof(cell) && (size_t)len>=size*sizeof(cell))
      len=size*sizeof(cell)-1;
    dest[len/sizeof(cell)]=0;   /* clear last bytes of last (semi-filled) cell*/
    #if defined AMX_ANSIONLY
      memcpy(dest,source,len);
    #else
      if (use_wchar) {
        for (i=0; i<len; i++)
          ((char*)dest)[i]=(char)(((wchar_t*)source)[i]);
      } else {
        memcpy(dest,source,len);
      } /* if */
    #endif
    /* On Big Endian machines, the characters are well aligned in the
     * cells; on Little Endian machines, we must swap all cells.
     */
    assert(check_endian());
    #if BYTE_ORDER==LITTLE_ENDIAN
      for (i=(int)(len/sizeof(cell)); i>=0; i--)
        amx_SwapCell((ucell *)&dest[i]);
    #endif
  } else {
    /* create an unpacked string */
    if (size<UNLIMITED && (size_t)len>=size)
      len=size-1;
    #if defined AMX_ANSIONLY
      for (i=0; i<len; i++)
        dest[i]=(cell)source[i];
    #else
      if (use_wchar) {
        for (i=0; i<len; i++)
          dest[i]=(cell)(((wchar_t*)source)[i]);
      } else {
        for (i=0; i<len; i++)
          dest[i]=(cell)source[i];
      } /* if */
    #endif
    dest[len]=0;
  } /* if */
  return AMX_ERR_NONE;
}
#endif

#if defined AMX_XXXSTRING
int AMXAPI amx_GetString(char *dest,const cell *source,int use_wchar,size_t size)
{
  int len=0;
  #if defined AMX_ANSIONLY
    (void)use_wchar;    /* unused parameter (if ANSI only) */
  #endif
  if ((ucell)*source>UNPACKEDMAX) {
    /* source string is packed */
    cell c=0;           /* initialize to 0 to avoid a compiler warning */
    int i=sizeof(cell)-1;
    char ch;
    while ((size_t)len<size) {
      if (i==sizeof(cell)-1)
        c=*source++;
      ch=(char)(c >> i*CHARBITS);
      if (ch=='\0')
        break;          /* terminating zero character found */
      #if defined AMX_ANSIONLY
        dest[len++]=ch;
      #else
        if (use_wchar)
          ((wchar_t*)dest)[len++]=ch;
        else
          dest[len++]=ch;
      #endif
      i=(i+sizeof(cell)-1) % sizeof(cell);
    } /* while */
  } else {
    /* source string is unpacked */
    #if defined AMX_ANSIONLY
      while (*source!=0 && (size_t)len<size)
        dest[len++]=(char)*source++;
    #else
      if (use_wchar) {
        while (*source!=0 && (size_t)len<size)
          ((wchar_t*)dest)[len++]=(wchar_t)*source++;
      } else {
        while (*source!=0 && (size_t)len<size)
          dest[len++]=(char)*source++;
      } /* if */
    #endif
  } /* if */
  /* store terminator */
  if ((size_t)len>=size)
    len=(int)size-1;
  if (len>=0) {
    #if defined AMX_ANSIONLY
      dest[len]='\0';
    #else
      if (use_wchar)
        ((wchar_t*)dest)[len]=0;
      else
        dest[len]='\0';
    #endif
  } /* IF */
  return AMX_ERR_NONE;
}
#endif /* AMX_XXXSTRING */

#if defined AMX_UTF8XXX
  #if defined __BORLANDC__
    #pragma warn -amb -8000     /* ambiguous operators need parentheses */
  #endif
/* amx_UTF8Get()
 * Extract a single UTF-8 encoded character from a string and return a pointer
 * to the character just behind that UTF-8 character. The parameters "endptr"
 * and "value" may be NULL.
 * If the code is not valid UTF-8, "endptr" has the value of the input
 * parameter "string" and "value" is zero.
 */
int AMXAPI amx_UTF8Get(const char *string, const char **endptr, cell *value)
{
static const char utf8_count[16]={ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 4 };
static const long utf8_lowmark[5] = { 0x80, 0x800, 0x10000L, 0x200000L, 0x4000000L };
  unsigned char c;
  cell result;
  int followup;

  assert(string!=NULL);
  if (value!=NULL)      /* preset, in case of an error */
    *value=0;
  if (endptr!=NULL)
    *endptr=string;

  c = *(const unsigned char*)string++;
  if (c<0x80) {
    /* ASCII */
    result=c;
  } else {
    if (c<0xc0 || c>=0xfe)
      return AMX_ERR_PARAMS;  /* invalid or "follower" code, quit with error */
    /* At this point we know that the two top bits of c are ones. The two
     * bottom bits are always part of the code. We only need to consider
     * the 4 remaining bits; i.e., a 16-byte table. This is "utf8_count[]".
     * (Actually the utf8_count[] table records the number of follow-up
     * bytes minus 1. This is just for convenience.)
     */
    assert((c & 0xc0)==0xc0);
    followup=(int)utf8_count[(c >> 2) & 0x0f];
    /* The mask depends on the code length; this is just a very simple
     * relation.
     */
    #define utf8_mask   (0x1f >> followup)
    result= c & utf8_mask;
    /* Collect the follow-up codes using a drop-through switch statement;
     * this avoids a loop. In each case, verify the two leading bits.
     */
    assert(followup>=0 && followup<=4);
    switch (followup) {
    case 4:
      if (((c=*string++) & 0xc0) != 0x80) goto error;
      result = (result << 6) | c & 0x3f;
    case 3:
      if (((c=*string++) & 0xc0) != 0x80) goto error;
      result = (result << 6) | c & 0x3f;
    case 2:
      if (((c=*string++) & 0xc0) != 0x80) goto error;
      result = (result << 6) | c & 0x3f;
    case 1:
      if (((c=*string++) & 0xc0) != 0x80) goto error;
      result = (result << 6) | c & 0x3f;
    case 0:
      if (((c=*string++) & 0xc0) != 0x80) goto error;
      result = (result << 6) | c & 0x3f;
    } /* switch */
    /* Do additional checks: shortest encoding & reserved positions. The
     * lowmark limits also depends on the code length; it can be read from
     * a table with 5 elements. This is "utf8_lowmark[]".
     */
    if (result<utf8_lowmark[followup])
      goto error;
    if (result>=0xd800 && result<=0xdfff || result==0xfffe || result==0xffff)
      goto error;
  } /* if */

  if (value!=NULL)
    *value=result;
  if (endptr!=NULL)
    *endptr=string;

  return AMX_ERR_NONE;

error:
  return AMX_ERR_PARAMS;
}

/* amx_UTF8Put()
 * Encode a single character into a byte string. The character may result in
 * a string of up to 6 bytes. The function returns an error code if "maxchars"
 * is lower than the required number of characters; in this case nothing is
 * stored.
 * The function does not zero-terminate the string.
 */
int AMXAPI amx_UTF8Put(char *string, char **endptr, int maxchars, cell value)
{
  assert(string!=NULL);
  if (endptr!=NULL)     /* preset, in case of an error */
    *endptr=string;

  if (value<0x80) {
    /* 0xxxxxxx */
    if (maxchars < 1) goto error;
    *string++ = (char)value;
  } else if (value<0x800) {
    /* 110xxxxx 10xxxxxx */
    if (maxchars < 2) goto error;
    *string++ = (char)((value>>6) & 0x1f | 0xc0);
    *string++ = (char)(value & 0x3f | 0x80);
  } else if (value<0x10000) {
    /* 1110xxxx 10xxxxxx 10xxxxxx (16 bits, BMP plane) */
    if (maxchars < 3) goto error;
    if (value>=0xd800 && value<=0xdfff || value==0xfffe || value==0xffff)
      goto error;       /* surrogate pairs and invalid characters */
    *string++ = (char)((value>>12) & 0x0f | 0xe0);
    *string++ = (char)((value>>6) & 0x3f | 0x80);
    *string++ = (char)(value & 0x3f | 0x80);
  } else if (value<0x200000) {
    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (maxchars < 4) goto error;
    *string++ = (char)((value>>18) & 0x07 | 0xf0);
    *string++ = (char)((value>>12) & 0x3f | 0x80);
    *string++ = (char)((value>>6) & 0x3f | 0x80);
    *string++ = (char)(value & 0x3f | 0x80);
  } else if (value<0x4000000) {
    /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (maxchars < 5) goto error;
    *string++ = (char)((value>>24) & 0x03 | 0xf8);
    *string++ = (char)((value>>18) & 0x3f | 0x80);
    *string++ = (char)((value>>12) & 0x3f | 0x80);
    *string++ = (char)((value>>6) & 0x3f | 0x80);
    *string++ = (char)(value & 0x3f | 0x80);
  } else {
    /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx (31 bits) */
    if (maxchars < 6) goto error;
    *string++ = (char)((value>>30) & 0x01 | 0xfc);
    *string++ = (char)((value>>24) & 0x3f | 0x80);
    *string++ = (char)((value>>18) & 0x3f | 0x80);
    *string++ = (char)((value>>12) & 0x3f | 0x80);
    *string++ = (char)((value>>6) & 0x3f | 0x80);
    *string++ = (char)(value & 0x3f | 0x80);
  } /* if */

  if (endptr!=NULL)
    *endptr=string;
  return AMX_ERR_NONE;

error:
  return AMX_ERR_PARAMS;
}

/* amx_UTF8Check()
 * Run through a zero-terminated string and check the validity of the UTF-8
 * encoding. The function returns an error code, it is AMX_ERR_NONE if the
 * string is valid UTF-8 (or valid ASCII for that matter).
 */
int AMXAPI amx_UTF8Check(const char *string, int *length)
{
  int err=AMX_ERR_NONE;
  int len=0;
  while (err==AMX_ERR_NONE && *string!='\0') {
    err=amx_UTF8Get(string,&string,NULL);
    len++;
  } /* while */
  if (length!=NULL)
    *length=len;
  return err;
}

/* amx_UTF8Len()
 * Run through a wide string and return how many 8-bit characters are needed to
 * store the string in UTF-8 format. The returned cound excludes the terminating
 * zero byte. The function returns an error code.
 */
int AMXAPI amx_UTF8Len(const cell *cstr, int *length)
{
  int err;

  assert(length!=NULL);
  err=amx_StrLen(cstr, length);
  if (err==AMX_ERR_NONE && (ucell)*cstr<=UNPACKEDMAX) {
    char buffer[10];  /* maximum UTF-8 code is 6 characters */
    char *endptr;
    int len=*length, count=0;
    while (len-->0) {
      amx_UTF8Put(buffer, &endptr, sizeof buffer, *cstr++);
      count+=(int)(endptr-buffer);
    } /* while */
    *length=count;
  } /* while */
  return err;
}
#endif /* AMX_UTF8XXX */
