/*  Example implementation of an optimized abstract machine.
 *
 *  This abstract machine uses the "labels as values" feature of the GNU GCC
 *  compiler to create a fast "indirect threaded" interpreter. The Intel C/C++
 *  compiler supports this too.
 *
 *  Copyright (c) ITB CompuPhase, 1998-2013
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id: amxcons.c 3821 2007-10-15 16:54:20Z thiadmer $
 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "osdefs.h"
#if defined __ECOS__
  /* eCos puts include files in cyg/package_name */
  #include <cyg/pawn/amx.h>
#else
  #include "amx.h"
#endif

#if !(defined __GNUC__ || defined __ICC)
  #error The GNU GCC or the Intel C/C++ compiler is required for this file.
#endif

#if !defined sizearray
  #define sizearray(a)  (sizeof(a)/sizeof((a)[0]))
#endif

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

#if !defined GETPARAM
  #define GETPARAM(v)   ( v=*cip++ )  /* read a parameter from the opcode stream */
#endif
#if !defined GETPARAM_P
  #define GETPARAM_P(v,o) ( v=((cell)(o) >> (int)(sizeof(cell)*4)) )
#endif
#if !defined SKIPPARAM
  #define SKIPPARAM(n)  ( cip=(cell *)cip+(n) ) /* for obsolete opcodes */
#endif

/* PUSH() and POP() are defined in terms of the _R() and _W() macros */
#define PUSH(v)         ( stk-=sizeof(cell), _W(data,stk,v) )
#define POP(v)          ( v=_R(data,stk), stk+=sizeof(cell) )

#define ABORT(amx,v)    { (amx)->stk=reset_stk; (amx)->hea=reset_hea; return v; }

#define STKMARGIN       ((cell)(16*sizeof(cell)))
#define CHKMARGIN()     if (hea+STKMARGIN>stk) return AMX_ERR_STACKERR
#define CHKSTACK()      if (stk>amx->stp) return AMX_ERR_STACKLOW
#define CHKHEAP()       if (hea<amx->hlw) return AMX_ERR_HEAPLOW

#define JUMPREL(ip)     ((cell*)((unsigned long)(ip)+*(cell*)(ip)-sizeof(cell)))


#if !defined AMX_NO_PACKED_OPC && !defined AMX_TOKENTHREADING
  #define AMX_TOKENTHREADING    /* packed opcodes require token threading */
#endif
#if defined AMX_TOKENTHREADING
  #if defined AMX_NO_PACKED_OPC
    #define NEXT(cip,op) goto *amx_opcodelist[*cip++]
  #else
    #define NEXT(cip,op) goto *amx_opcodelist[(op=*cip++) & ((1 << sizeof(cell)*4)-1)]
  #endif
#else
  #if !defined AMX_NO_PACKED_OPC
    #error Packed opcodes support requires token threading
  #endif
  #define NEXT(cip,op)   goto **cip++
#endif

cell amx_exec_run(AMX *amx,cell *retval,unsigned char *data)
{
static const void * const amx_opcodelist[] = {
        /* core set */
        &&op_nop,         &&op_load_pri,    &&op_load_alt,    &&op_load_s_pri,
        &&op_load_s_alt,  &&op_lref_s_pri,  &&op_lref_s_alt,  &&op_load_i,
        &&op_lodb_i,      &&op_const_pri,   &&op_const_alt,   &&op_addr_pri,
        &&op_addr_alt,    &&op_stor,        &&op_stor_s,      &&op_sref_s,
        &&op_stor_i,      &&op_strb_i,      &&op_align_pri,   &&op_lctrl,
        &&op_sctrl,       &&op_xchg,        &&op_push_pri,    &&op_push_alt,
        &&op_pushr_pri,   &&op_pop_pri,     &&op_pop_alt,     &&op_pick,
        &&op_stack,       &&op_heap,        &&op_proc,        &&op_ret,
        &&op_retn,        &&op_call,        &&op_jump,        &&op_jzer,
        &&op_jnz,         &&op_shl,         &&op_shr,         &&op_sshr,
        &&op_shl_c_pri,   &&op_shl_c_alt,   &&op_smul,        &&op_sdiv,
        &&op_add,         &&op_sub,         &&op_and,         &&op_or,
        &&op_xor,         &&op_not,         &&op_neg,         &&op_invert,
        &&op_eq,          &&op_neq,         &&op_sless,       &&op_sleq,
        &&op_sgrtr,       &&op_sgeq,        &&op_inc_pri,     &&op_inc_alt,
        &&op_inc_i,       &&op_dec_pri,     &&op_dec_alt,     &&op_dec_i,
        &&op_movs,        &&op_cmps,        &&op_fill,        &&op_halt,
        &&op_bounds,      &&op_sysreq,      &&op_switch,      &&op_swap_pri,
        &&op_swap_alt,    &&op_break,       &&op_casetbl,
        /* patched instructions */
        /* if op_sysreq_d and/or op_sysreq_nd are not implemented, their entries
         * in this table must be NULL
         */
        &&op_sysreq_d,    &&op_sysreq_nd,
        /* overlay instructions */
        &&op_call_ovl,    &&op_retn_ovl,    &&op_switch_ovl,  &&op_casetbl_ovl,
        /* supplemental and macro instructions */
#if !defined AMX_NO_MACRO_INSTR
        &&op_lidx,        &&op_lidx_b,      &&op_idxaddr,     &&op_idxaddr_b,
        &&op_push_c,      &&op_push,        &&op_push_s,      &&op_push_adr,
        &&op_pushr_c,     &&op_pushr_s,     &&op_pushr_adr,   &&op_jeq,
        &&op_jneq,        &&op_jsless,      &&op_jsleq,       &&op_jsgrtr,
        &&op_jsgeq,       &&op_sdiv_inv,    &&op_sub_inv,     &&op_add_c,
        &&op_smul_c,      &&op_zero_pri,    &&op_zero_alt,    &&op_zero,
        &&op_zero_s,      &&op_eq_c_pri,    &&op_eq_c_alt,    &&op_inc,
        &&op_inc_s,       &&op_dec,         &&op_dec_s,       &&op_sysreq_n,
        &&op_pushm_c,     &&op_pushm,       &&op_pushm_s,     &&op_pushm_adr,
        &&op_pushrm_c,    &&op_pushrm_s,    &&op_pushrm_adr,  &&op_load2,
        &&op_load2_s,     &&op_const,       &&op_const_s,
#endif
#if !defined AMX_NO_PACKED_OPC
        &&op_load_p_pri,  &&op_load_p_alt,  &&op_load_p_s_pri,&&op_load_p_s_alt,
        &&op_lref_p_s_pri,&&op_lref_p_s_alt,&&op_lodb_p_i,    &&op_const_p_pri,
        &&op_const_p_alt, &&op_addr_p_pri,  &&op_addr_p_alt,  &&op_stor_p,
        &&op_stor_p_s,    &&op_sref_p_s,    &&op_strb_p_i,    &&op_lidx_p_b,
        &&op_idxaddr_p_b, &&op_align_p_pri, &&op_push_p_c,    &&op_push_p,
        &&op_push_p_s,    &&op_push_p_adr,  &&op_pushr_p_c,   &&op_pushr_p_s,
        &&op_pushr_p_adr, &&op_pushm_p_c,   &&op_pushm_p,     &&op_pushm_p_s,
        &&op_pushm_p_adr, &&op_pushrm_p_c,  &&op_pushrm_p_s,  &&op_pushrm_p_adr,
        &&op_stack_p,     &&op_heap_p,      &&op_shl_p_c_pri, &&op_shl_p_c_alt,
        &&op_add_p_c,     &&op_smul_p_c,    &&op_zero_p,      &&op_zero_p_s,
        &&op_eq_p_c_pri,  &&op_eq_p_c_alt,  &&op_inc_p,       &&op_inc_p_s,
        &&op_dec_p,       &&op_dec_p_s,     &&op_movs_p,      &&op_cmps_p,
        &&op_fill_p,      &&op_halt_p,      &&op_bounds_p
#endif
};
  AMX_HEADER *hdr;
  cell pri,alt,stk,frm,hea;
  cell reset_stk, reset_hea, *cip;
  cell offs,val;
  int num,i;
  #if !defined AMX_NO_PACKED_OPC
    int op;
  #endif

  assert(amx!=NULL);
  /* HACK: return label table and opcode count (for VerifyPcode()) if amx
   * structure has the flags set to all ones (see amx_exec_list() above)
   */
  if (amx->flags==~0) {
    assert(sizeof(cell)==sizeof(void *));
    assert(data==NULL);
    assert(retval!=NULL);
    *retval=(cell)amx_opcodelist;
    return sizearray(amx_opcodelist);
  } /* if */

  /* set up the registers */
  hdr=(AMX_HEADER *)amx->base;
  assert(hdr->magic==AMX_MAGIC);
  assert(hdr->file_version>=11);
  cip=(cell*)amx->cip;
  hea=amx->hea;
  stk=amx->stk;
  reset_stk=stk;
  reset_hea=hea;
  alt=frm=pri=0;/* just to avoid compiler warnings */
  num=0;        /* just to avoid compiler warnings */

  /* start running */
  assert(amx->code!=NULL);
  assert(data!=NULL);
  cip=(cell *)(amx->code+(int)amx->cip);
  NEXT(cip,op);

  op_nop:
    NEXT(cip,op);
  op_load_pri:
    GETPARAM(offs);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_load_alt:
    GETPARAM(offs);
    alt=_R(data,offs);
    NEXT(cip,op);
  op_load_s_pri:
    GETPARAM(offs);
    pri=_R(data,frm+offs);
    NEXT(cip,op);
  op_load_s_alt:
    GETPARAM(offs);
    alt=_R(data,frm+offs);
    NEXT(cip,op);
  op_lref_s_pri:
    GETPARAM(offs);
    offs=_R(data,frm+offs);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_lref_s_alt:
    GETPARAM(offs);
    offs=_R(data,frm+offs);
    alt=_R(data,offs);
    NEXT(cip,op);
  op_load_i:
    /* verify address */
    if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    pri=_R(data,pri);
    NEXT(cip,op);
  op_lodb_i:
    GETPARAM(offs);
  __lodb_i:
    /* verify address */
    if (pri>=hea && pri<stk || (ucell)pri>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    switch (offs) {
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
    NEXT(cip,op);
  op_const_pri:
    GETPARAM(pri);
    NEXT(cip,op);
  op_const_alt:
    GETPARAM(alt);
    NEXT(cip,op);
  op_addr_pri:
    GETPARAM(pri);
    pri+=frm;
    NEXT(cip,op);
  op_addr_alt:
    GETPARAM(alt);
    alt+=frm;
    NEXT(cip,op);
  op_stor:
    GETPARAM(offs);
    _W(data,offs,pri);
    NEXT(cip,op);
  op_stor_s:
    GETPARAM(offs);
    _W(data,frm+offs,pri);
    NEXT(cip,op);
  op_sref_s:
    GETPARAM(offs);
    offs=_R(data,frm+offs);
    _W(data,offs,pri);
    NEXT(cip,op);
  op_stor_i:
    /* verify address */
    if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    _W(data,alt,pri);
    NEXT(cip,op);
  op_strb_i:
    GETPARAM(offs);
  __strb_i:
    /* verify address */
    if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    switch (offs) {
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
    NEXT(cip,op);
  op_align_pri:
    GETPARAM(offs);
    #if BYTE_ORDER==LITTLE_ENDIAN
      if (offs<(int)sizeof(cell))
        pri ^= sizeof(cell)-offs;
    #endif
    NEXT(cip,op);
  op_lctrl:
    GETPARAM(offs);
    switch (offs) {
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
      pri=(cell)((unsigned char *)cip - amx->code);
      break;
    } /* switch */
    NEXT(cip,op);
  op_sctrl:
    GETPARAM(offs);
    switch (offs) {
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
    NEXT(cip,op);
  op_xchg:
    offs=pri;         /* offs is a temporary variable */
    pri=alt;
    alt=offs;
    NEXT(cip,op);
  op_push_pri:
    PUSH(pri);
    NEXT(cip,op);
  op_push_alt:
    PUSH(alt);
    NEXT(cip,op);
  op_pushr_pri:
    PUSH(data+pri);
    NEXT(cip,op);
  op_pop_pri:
    POP(pri);
    NEXT(cip,op);
  op_pop_alt:
    POP(alt);
    NEXT(cip,op);
  op_pick:
    GETPARAM(offs);
    pri=_R(data,stk+offs);
    NEXT(cip,op);
  op_stack:
    GETPARAM(offs);
    alt=stk;
    stk+=offs;
    CHKMARGIN();
    CHKSTACK();
    NEXT(cip,op);
  op_heap:
    GETPARAM(offs);
    alt=hea;
    hea+=offs;
    CHKMARGIN();
    CHKHEAP();
    NEXT(cip,op);
  op_proc:
    PUSH(frm);
    frm=stk;
    CHKMARGIN();
    NEXT(cip,op);
  op_ret:
    POP(frm);
    POP(offs);
    /* verify the return address */
    if ((long)offs>=amx->codesize)
      ABORT(amx,AMX_ERR_MEMACCESS);
    cip=(cell *)(amx->code+(int)offs);
    NEXT(cip,op);
  op_retn:
    POP(frm);
    POP(offs);
    /* verify the return address */
    if ((long)offs>=amx->codesize)
      ABORT(amx,AMX_ERR_MEMACCESS);
    cip=(cell *)(amx->code+(int)offs);
    stk+= _R(data,stk) + sizeof(cell);  /* remove parameters from the stack */
    NEXT(cip,op);
  op_call:
    PUSH(((unsigned char *)cip-amx->code)+sizeof(cell));/* push address behind instruction */
    cip=JUMPREL(cip);                   /* jump to the address */
    NEXT(cip,op);
  op_jump:
    /* since the GETPARAM() macro modifies cip, you cannot
     * do GETPARAM(cip) directly */
    cip=JUMPREL(cip);
    NEXT(cip,op);
  op_jzer:
    if (pri==0)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jnz:
    if (pri!=0)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_shl:
    pri<<=alt;
    NEXT(cip,op);
  op_shr:
    pri=(ucell)pri >> (ucell)alt;
    NEXT(cip,op);
  op_sshr:
    pri>>=alt;
    NEXT(cip,op);
  op_shl_c_pri:
    GETPARAM(offs);
    pri<<=offs;
    NEXT(cip,op);
  op_shl_c_alt:
    GETPARAM(offs);
    alt<<=offs;
    NEXT(cip,op);
  op_smul:
    pri*=alt;
    NEXT(cip,op);
  op_sdiv:
    if (pri==0)
      ABORT(amx,AMX_ERR_DIVIDE);
    /* use floored division and matching remainder */
    offs=pri;
    #if defined TRUNC_SDIV
      pri=alt/offs;
      alt=alt%offs;
    #else
      val=alt;                  /* portable routine for truncated division */
      pri=IABS(alt)/IABS(offs);
      if ((cell)(val ^ offs)<0)
        pri=-pri;
      alt=val-pri*offs;         /* calculate the matching remainder */
    #endif
    /* now "fiddle" with the values to get floored division */
    if (alt!=0 && (cell)(alt ^ offs)<0) {
      pri--;
      alt+=offs;
    } /* if */
    NEXT(cip,op);
  op_add:
    pri+=alt;
    NEXT(cip,op);
  op_sub:
    pri=alt-pri;
    NEXT(cip,op);
  op_and:
    pri&=alt;
    NEXT(cip,op);
  op_or:
    pri|=alt;
    NEXT(cip,op);
  op_xor:
    pri^=alt;
    NEXT(cip,op);
  op_not:
    pri=!pri;
    NEXT(cip,op);
  op_neg:
    pri=-pri;
    NEXT(cip,op);
  op_invert:
    pri=~pri;
    NEXT(cip,op);
  op_eq:
    pri= pri==alt ? 1 : 0;
    NEXT(cip,op);
  op_neq:
    pri= pri!=alt ? 1 : 0;
    NEXT(cip,op);
  op_sless:
    pri= pri<alt ? 1 : 0;
    NEXT(cip,op);
  op_sleq:
    pri= pri<=alt ? 1 : 0;
    NEXT(cip,op);
  op_sgrtr:
    pri= pri>alt ? 1 : 0;
    NEXT(cip,op);
  op_sgeq:
    pri= pri>=alt ? 1 : 0;
    NEXT(cip,op);
  op_inc_pri:
    pri++;
    NEXT(cip,op);
  op_inc_alt:
    alt++;
    NEXT(cip,op);
  op_inc_i:
    #if defined _R_DEFAULT
      *(cell *)(data+(int)pri) += 1;
    #else
      val=_R(data,pri);
      _W(data,pri,val+1);
    #endif
    NEXT(cip,op);
  op_dec_pri:
    pri--;
    NEXT(cip,op);
  op_dec_alt:
    alt--;
    NEXT(cip,op);
  op_dec_i:
    #if defined _R_DEFAULT
      *(cell *)(data+(int)pri) -= 1;
    #else
      val=_R(data,pri);
      _W(data,pri,val-1);
    #endif
    NEXT(cip,op);
  op_movs:
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
    NEXT(cip,op);
  op_cmps:
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
    NEXT(cip,op);
  op_fill:
    GETPARAM(offs);
  __fill:
    /* verify top & bottom memory addresses */
    if (alt>=hea && alt<stk || (ucell)alt>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    if ((alt+offs)>hea && (alt+offs)<stk || (ucell)(alt+offs)>(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    for (i=(int)alt; offs>=(int)sizeof(cell); i+=sizeof(cell), offs-=sizeof(cell))
      _W32(data,i,pri);
    NEXT(cip,op);
  op_halt:
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
  op_bounds:
    GETPARAM(offs);
    if ((ucell)pri>(ucell)offs) {
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      ABORT(amx,AMX_ERR_BOUNDS);
    } /* if */
    NEXT(cip,op);
  op_sysreq:
    GETPARAM(offs);
    /* save a few registers */
    amx->cip=(cell)((unsigned char *)cip-amx->code);
    amx->hea=hea;
    amx->frm=frm;
    amx->stk=stk;
    num=amx->callback(amx,offs,&pri,(cell *)(data+(int)stk));
    if (num!=AMX_ERR_NONE) {
      if (num==AMX_ERR_SLEEP) {
        amx->pri=pri;
        amx->alt=alt;
        amx->reset_stk=reset_stk;
        amx->reset_hea=reset_hea;
        return num;
      } /* if */
      ABORT(amx,num);
    } /* if */
    NEXT(cip,op);
  op_switch: {
    cell *cptr=JUMPREL(cip)+1;  /* +1, to skip the "casetbl" opcode */
    cip=JUMPREL(cptr+1);        /* preset to "none-matched" case */
    num=(int)*cptr;             /* number of records in the case table */
    for (cptr+=2; num>0 && *cptr!=pri; num--,cptr+=2)
      /* nothing */;
    if (num>0)
      cip=JUMPREL(cptr+1);      /* case found */
    NEXT(cip,op);
    }
  op_swap_pri:
    offs=_R(data,stk);
    _W(data,stk,pri);
    pri=offs;
    NEXT(cip,op);
  op_swap_alt:
    offs=_R(data,stk);
    _W(data,stk,alt);
    alt=offs;
    NEXT(cip,op);
  op_break:
    assert((amx->flags & AMX_FLAG_VERIFY)==0);
    if (amx->debug!=NULL) {
      /* store status */
      amx->frm=frm;
      amx->stk=stk;
      amx->hea=hea;
      amx->cip=(cell)((unsigned char*)cip-amx->code);
      num=amx->debug(amx);
      if (num!=AMX_ERR_NONE) {
        if (num==AMX_ERR_SLEEP) {
          amx->pri=pri;
          amx->alt=alt;
          amx->reset_stk=reset_stk;
          amx->reset_hea=reset_hea;
          return num;
        } /* if */
        ABORT(amx,num);
      } /* if */
    } /* if */
    NEXT(cip,op);
  op_casetbl:
    assert(0);                  /* this should not occur during execution */
    ABORT(amx,AMX_ERR_INVINSTR);
  op_sysreq_d:          /* see op_sysreq */
    #if !defined AMX_DONT_RELOCATE
      GETPARAM(offs);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      pri=((AMX_NATIVE)offs)(amx,(cell *)(data+(int)stk));
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
      NEXT(cip,op);
    #else
      ABORT(amx,AMX_ERR_INVINSTR);
    #endif
  op_sysreq_nd:    /* see op_sysreq_n */
    #if !defined AMX_NO_MACRO_INSTR && !defined AMX_DONT_RELOCATE
      GETPARAM(offs);
      GETPARAM(val);
      PUSH(val);
      /* save a few registers */
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      amx->hea=hea;
      amx->frm=frm;
      amx->stk=stk;
      pri=((AMX_NATIVE)offs)(amx,(cell *)(data+(int)stk));
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
      NEXT(cip,op);
    #else
      ABORT(amx,AMX_ERR_INVINSTR);
    #endif

    /* overlay instructions */
#if !defined AMX_NO_OVERLAY
  op_call_ovl:
    offs=(unsigned char *)cip-amx->code+sizeof(cell); /* skip address */
    assert(offs>=0 && offs<(1<<(sizeof(cell)*4)));
    PUSH((offs<<(sizeof(cell)*4)) | amx->ovl_index);
    amx->ovl_index=(int)*cip;
    assert(amx->overlay!=NULL);
    if ((num=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
      ABORT(amx,num);
    cip=(cell*)amx->code;
    NEXT(cip,op);
  op_retn_ovl:
    assert(amx->overlay!=NULL);
    POP(frm);
    POP(offs);
    amx->ovl_index=offs & (((ucell)~0)>>4*sizeof(cell));
    offs=(ucell)offs >> (sizeof(cell)*4);
    /* verify the index */
    stk+=_R(data,stk)+sizeof(cell);   /* remove parameters from the stack */
    num=amx->overlay(amx,amx->ovl_index); /* reload overlay */
    if (num!=AMX_ERR_NONE || (long)offs>=amx->codesize)
      ABORT(amx,AMX_ERR_MEMACCESS);
    cip=(cell *)(amx->code+(int)offs);
    NEXT(cip,op);
  op_switch_ovl: {
    cell *cptr=JUMPREL(cip)+1;  /* +1, to skip the "icasetbl" opcode */
    amx->ovl_index=*(cptr+1);   /* preset to "none-matched" case */
    num=(int)*cptr;             /* number of records in the case table */
    for (cptr+=2; num>0 && *cptr!=pri; num--,cptr+=2)
      /* nothing */;
    if (num>0)
      amx->ovl_index=*(cptr+1); /* case found */
    assert(amx->overlay!=NULL);
    if ((num=amx->overlay(amx,amx->ovl_index))!=AMX_ERR_NONE)
      ABORT(amx,num);
    cip=(cell*)amx->code;
    NEXT(cip,op);
    }
#else
  op_call_ovl:
  op_retn_ovl:
  op_switch_ovl:
    ABORT(amx,AMX_ERR_INVINSTR);
#endif
  op_casetbl_ovl:
    assert(0);                  /* this should not occur during execution */
    ABORT(amx,AMX_ERR_INVINSTR);

    /* supplemental and macro instructions */
#if !defined AMX_NO_MACRO_INSTR
  op_lidx:
    offs=pri*sizeof(cell)+alt;  /* implicit shift value for a cell */
    /* verify address */
    if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_lidx_b:
    GETPARAM(offs);
    offs=(pri << (int)offs)+alt;
    /* verify address */
    if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_idxaddr:
    pri=pri*sizeof(cell)+alt;
    NEXT(cip,op);
  op_idxaddr_b:
    GETPARAM(offs);
    pri=(pri << (int)offs)+alt;
    NEXT(cip,op);
  op_push_c:
    GETPARAM(offs);
    PUSH(offs);
    NEXT(cip,op);
  op_push:
    GETPARAM(offs);
    PUSH(_R(data,offs));
    NEXT(cip,op);
  op_push_s:
    GETPARAM(offs);
    PUSH(_R(data,frm+offs));
    NEXT(cip,op);
  op_push_adr:
    GETPARAM(offs);
    PUSH(frm+offs);
    NEXT(cip,op);
  op_pushr_c:
    GETPARAM(offs);
    PUSH(data+offs);
    NEXT(cip,op);
  op_pushr_s:
    GETPARAM(offs);
    PUSH(data+_R(data,frm+offs));
    NEXT(cip,op);
  op_pushr_adr:
    GETPARAM(offs);
    PUSH(data+frm+offs);
    NEXT(cip,op);
  op_jeq:
    if (pri==alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jneq:
    if (pri!=alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jsless:
    if (pri<alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jsleq:
    if (pri<=alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jsgrtr:
    if (pri>alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_jsgeq:
    if (pri>=alt)
      cip=JUMPREL(cip);
    else
      SKIPPARAM(1);
    NEXT(cip,op);
  op_sdiv_inv:
    if (alt==0)
      ABORT(amx,AMX_ERR_DIVIDE);
    /* use floored division and matching remainder */
    offs=alt;
    #if defined TRUNC_SDIV
      pri=pri/offs;
      alt=pri%offs;
    #else
      val=pri;                  /* portable routine for truncated division */
      pri=IABS(pri)/IABS(offs);
      if ((cell)(val ^ offs)<0)
        pri=-pri;
      alt=val-pri*offs;         /* calculate the matching remainder */
    #endif
    /* now "fiddle" with the values to get floored division */
    if (alt!=0 && (cell)(alt ^ offs)<0) {
      pri--;
      alt+=offs;
    } /* if */
    NEXT(cip,op);
  op_sub_inv:
    pri-=alt;
    NEXT(cip,op);
  op_add_c:
    GETPARAM(offs);
    pri+=offs;
    NEXT(cip,op);
  op_smul_c:
    GETPARAM(offs);
    pri*=offs;
    NEXT(cip,op);
  op_zero_pri:
    pri=0;
    NEXT(cip,op);
  op_zero_alt:
    alt=0;
    NEXT(cip,op);
  op_zero:
    GETPARAM(offs);
    _W(data,offs,0);
    NEXT(cip,op);
  op_zero_s:
    GETPARAM(offs);
    _W(data,frm+offs,0);
    NEXT(cip,op);
  op_eq_c_pri:
    GETPARAM(offs);
    pri= pri==offs ? 1 : 0;
    NEXT(cip,op);
  op_eq_c_alt:
    GETPARAM(offs);
    pri= alt==offs ? 1 : 0;
    NEXT(cip,op);
  op_inc:
    GETPARAM(offs);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)offs) += 1;
    #else
      val=_R(data,offs);
      _W(data,offs,val+1);
    #endif
    NEXT(cip,op);
  op_inc_s:
    GETPARAM(offs);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)(frm+offs)) += 1;
    #else
      val=_R(data,frm+offs);
      _W(data,frm+offs,val+1);
    #endif
    NEXT(cip,op);
  op_dec:
    GETPARAM(offs);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)offs) -= 1;
    #else
      val=_R(data,offs);
      _W(data,offs,val-1);
    #endif
    NEXT(cip,op);
  op_dec_s:
    GETPARAM(offs);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)(frm+offs)) -= 1;
    #else
      val=_R(data,frm+offs);
      _W(data,frm+offs,val-1);
    #endif
    NEXT(cip,op);
  op_sysreq_n:
    GETPARAM(offs);
    GETPARAM(val);
    PUSH(val);
    /* save a few registers */
    amx->cip=(cell)((unsigned char *)cip-amx->code);
    amx->hea=hea;
    amx->frm=frm;
    amx->stk=stk;
    num=amx->callback(amx,offs,&pri,(cell *)(data+(int)stk));
    stk+=val+4;
    if (num!=AMX_ERR_NONE) {
      if (num==AMX_ERR_SLEEP) {
        amx->pri=pri;
        amx->alt=alt;
        amx->stk=stk;
        amx->reset_stk=reset_stk;
        amx->reset_hea=reset_hea;
        return num;
      } /* if */
      ABORT(amx,num);
    } /* if */
    NEXT(cip,op);
  op_pushm_c:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(offs);
    } /* while */
    NEXT(cip,op);
  op_pushm:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(_R(data,offs));
    } /* while */
    NEXT(cip,op);
  op_pushm_s:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(_R(data,frm+offs));
    } /* while */
    NEXT(cip,op);
  op_pushm_adr:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(frm+offs);
    } /* while */
    NEXT(cip,op);
  op_pushrm_c:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+offs);
    } /* while */
    NEXT(cip,op);
  op_pushrm_s:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+_R(data,frm+offs));
    } /* while */
    NEXT(cip,op);
  op_pushrm_adr:
    GETPARAM(val);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+frm+offs);
    } /* while */
    NEXT(cip,op);
  op_load2:
    GETPARAM(offs);
    pri=_R(data,offs);
    GETPARAM(offs);
    alt=_R(data,offs);
    NEXT(cip,op);
  op_load2_s:
    GETPARAM(offs);
    pri=_R(data,frm+offs);
    GETPARAM(offs);
    alt=_R(data,frm+offs);
    NEXT(cip,op);
  op_const:
    GETPARAM(offs);
    GETPARAM(val);
    _W(data,offs,val);
    NEXT(cip,op);
  op_const_s:
    GETPARAM(offs);
    GETPARAM(val);
    _W(data,frm+offs,val);
    NEXT(cip,op);
#endif

#if !defined AMX_NO_PACKED_OPC
  op_load_p_pri:
    GETPARAM_P(offs,op);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_load_p_alt:
    GETPARAM_P(offs,op);
    alt=_R(data,offs);
    NEXT(cip,op);
  op_load_p_s_pri:
    GETPARAM_P(offs,op);
    pri=_R(data,frm+offs);
    NEXT(cip,op);
  op_load_p_s_alt:
    GETPARAM_P(offs,op);
    alt=_R(data,frm+offs);
    NEXT(cip,op);
  op_lref_p_s_pri:
    GETPARAM_P(offs,op);
    offs=_R(data,frm+offs);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_lref_p_s_alt:
    GETPARAM_P(offs,op);
    offs=_R(data,frm+offs);
    alt=_R(data,offs);
    NEXT(cip,op);
  op_lodb_p_i:
    GETPARAM_P(offs,op);
    goto __lodb_i;
  op_const_p_pri:
    GETPARAM_P(pri,op);
    NEXT(cip,op);
  op_const_p_alt:
    GETPARAM_P(alt,op);
    NEXT(cip,op);
  op_addr_p_pri:
    GETPARAM_P(pri,op);
    pri+=frm;
    NEXT(cip,op);
  op_addr_p_alt:
    GETPARAM_P(alt,op);
    alt+=frm;
    NEXT(cip,op);
  op_stor_p:
    GETPARAM_P(offs,op);
    _W(data,offs,pri);
    NEXT(cip,op);
  op_stor_p_s:
    GETPARAM_P(offs,op);
    _W(data,frm+offs,pri);
    NEXT(cip,op);
  op_sref_p_s:
    GETPARAM_P(offs,op);
    offs=_R(data,frm+offs);
    _W(data,offs,pri);
    NEXT(cip,op);
  op_strb_p_i:
    GETPARAM_P(offs,op);
    goto __strb_i;
  op_lidx_p_b:
    GETPARAM_P(offs,op);
    offs=(pri << (int)offs)+alt;
    /* verify address */
    if (offs>=hea && offs<stk || (ucell)offs>=(ucell)amx->stp)
      ABORT(amx,AMX_ERR_MEMACCESS);
    pri=_R(data,offs);
    NEXT(cip,op);
  op_idxaddr_p_b:
    GETPARAM_P(offs,op);
    pri=(pri << (int)offs)+alt;
    NEXT(cip,op);
  op_align_p_pri:
    GETPARAM_P(offs,op);
    #if BYTE_ORDER==LITTLE_ENDIAN
      if ((size_t)offs<sizeof(cell))
        pri ^= sizeof(cell)-offs;
    #endif
    NEXT(cip,op);
  op_push_p_c:
    GETPARAM_P(offs,op);
    PUSH(offs);
    NEXT(cip,op);
  op_push_p:
    GETPARAM_P(offs,op);
    PUSH(_R(data,offs));
    NEXT(cip,op);
  op_push_p_s:
    GETPARAM_P(offs,op);
    PUSH(_R(data,frm+offs));
    NEXT(cip,op);
  op_push_p_adr:
    GETPARAM_P(offs,op);
    PUSH(frm+offs);
    NEXT(cip,op);
  op_pushr_p_c:
    GETPARAM_P(offs,op);
    PUSH(data+offs);
    NEXT(cip,op);
  op_pushr_p_s:
    GETPARAM_P(offs,op);
    PUSH(data+_R(data,frm+offs));
    NEXT(cip,op);
  op_pushr_p_adr:
    GETPARAM_P(offs,op);
    PUSH(data+frm+offs);
    NEXT(cip,op);
  op_pushm_p_c:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(offs);
    } /* while */
    NEXT(cip,op);
  op_pushm_p:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(_R(data,offs));
    } /* while */
    NEXT(cip,op);
  op_pushm_p_s:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(_R(data,frm+offs));
    } /* while */
    NEXT(cip,op);
  op_pushm_p_adr:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(frm+offs);
    } /* while */
    NEXT(cip,op);
  op_pushrm_p_c:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+offs);
    } /* while */
    NEXT(cip,op);
  op_pushrm_p_s:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+_R(data,frm+offs));
    } /* while */
    NEXT(cip,op);
  op_pushrm_p_adr:
    GETPARAM_P(val,op);
    while (val--) {
      GETPARAM(offs);
      PUSH(data+frm+offs);
    } /* while */
    NEXT(cip,op);
  op_stack_p:
    GETPARAM_P(offs,op);
    alt=stk;
    stk+=offs;
    CHKMARGIN();
    CHKSTACK();
    NEXT(cip,op);
  op_heap_p:
    GETPARAM_P(offs,op);
    alt=hea;
    hea+=offs;
    CHKMARGIN();
    CHKHEAP();
    NEXT(cip,op);
  op_shl_p_c_pri:
    GETPARAM_P(offs,op);
    pri<<=offs;
    NEXT(cip,op);
  op_shl_p_c_alt:
    GETPARAM_P(offs,op);
    alt<<=offs;
    NEXT(cip,op);
  op_add_p_c:
    GETPARAM_P(offs,op);
    pri+=offs;
    NEXT(cip,op);
  op_smul_p_c:
    GETPARAM_P(offs,op);
    pri*=offs;
    NEXT(cip,op);
  op_zero_p:
    GETPARAM_P(offs,op);
    _W(data,offs,0);
    NEXT(cip,op);
  op_zero_p_s:
    GETPARAM_P(offs,op);
    _W(data,frm+offs,0);
    NEXT(cip,op);
  op_eq_p_c_pri:
    GETPARAM_P(offs,op);
    pri= pri==offs ? 1 : 0;
    NEXT(cip,op);
  op_eq_p_c_alt:
    GETPARAM_P(offs,op);
    pri= alt==offs ? 1 : 0;
    NEXT(cip,op);
  op_inc_p:
    GETPARAM_P(offs,op);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)offs) += 1;
    #else
      val=_R(data,offs);
      _W(data,offs,val+1);
    #endif
    NEXT(cip,op);
  op_inc_p_s:
    GETPARAM_P(offs,op);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)(frm+offs)) += 1;
    #else
      val=_R(data,frm+offs);
      _W(data,frm+offs,val+1);
    #endif
    NEXT(cip,op);
  op_dec_p:
    GETPARAM_P(offs,op);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)offs) -= 1;
    #else
      val=_R(data,offs);
      _W(data,offs,val-1);
    #endif
    NEXT(cip,op);
  op_dec_p_s:
    GETPARAM_P(offs,op);
    #if defined _R_DEFAULT
      *(cell *)(data+(int)(frm+offs)) -= 1;
    #else
      val=_R(data,frm+offs);
      _W(data,frm+offs,val-1);
    #endif
    NEXT(cip,op);
  op_movs_p:
    GETPARAM_P(offs,op);
    goto __movs;
  op_cmps_p:
    GETPARAM_P(offs,op);
    goto __cmps;
  op_fill_p:
    GETPARAM_P(offs,op);
    goto __fill;
  op_halt_p:
    GETPARAM_P(offs,op);
    goto __halt;
  op_bounds_p:
    GETPARAM_P(offs,op);
    if ((ucell)pri>(ucell)offs) {
      amx->cip=(cell)((unsigned char *)cip-amx->code);
      ABORT(amx,AMX_ERR_BOUNDS);
    } /* if */
    NEXT(cip,op);
#endif
}

void amx_exec_list(const AMX *amx,const cell **opcodelist,int *numopcodes)
{
  /* since the opcode list of the GNU GCC version of the abstract machine core
   * must be a local variable (as it references code labels, which are local
   * too), we use a trick to get the opcode list: call amx_Exec() while a
   * special flags value is set in the AMX header.
   */
   int orgflags;
   AMX *amxptr=(AMX*)amx;

   assert(amx!=NULL);
   assert(opcodelist!=NULL);
   assert(numopcodes!=NULL);
   orgflags=amx->flags;
   amxptr->flags=~0;
   *numopcodes=amx_exec_run(amxptr, (cell*)opcodelist, NULL);
   amxptr->flags=orgflags;
}
