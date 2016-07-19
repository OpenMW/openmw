/*  Float arithmetic for the Pawn Abstract Machine
 *
 *  Copyright (c) Artran, Inc. 1999
 *  Written by Greg Garner (gmg@artran.com)
 *  This file may be freely used. No warranties of any kind.
 *
 * CHANGES -
 * 2002-08-27: Basic conversion of source from C++ to C by Adam D. Moss
 *             <adam@gimp.org> <aspirin@icculus.org>
 * 2003-08-29: Removal of the dynamic memory allocation and replacing two
 *             type conversion functions by macros, by Thiadmer Riemersma
 * 2003-09-22: Moved the type conversion macros to AMX.H, and simplifications
 *             of some routines, by Thiadmer Riemersma
 * 2003-11-24: A few more native functions (geometry), plus minor modifications,
 *             mostly to be compatible with dynamically loadable extension
 *             modules, by Thiadmer Riemersma
 * 2004-01-09: Adaptions for 64-bit cells (using "double precision"), by
 *             Thiadmer Riemersma
 */
#include <stdlib.h>     /* for atof() */
#include <stdio.h>      /* for NULL */
#include <assert.h>
#include <math.h>
#include "amx.h"

/*
  #if defined __BORLANDC__
    #pragma resource "amxFloat.res"
  #endif
*/

#if PAWN_CELL_SIZE==32
  #define REAL          float
#elif PAWN_CELL_SIZE==64
  #define REAL          double
#else
  #error Unsupported cell size
#endif

#define PI  3.1415926535897932384626433832795

/******************************************************************/
static cell AMX_NATIVE_CALL n_float(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = integer value to convert to a float
    */
    REAL fValue;

    (void)amx;
    /* Convert to a float. Calls the compilers long to float conversion. */
    fValue = (REAL) params[1];

    /* Return the cell. */
    return amx_ftoc(fValue);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_strfloat(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = virtual string address to convert to a float
    */
    char szSource[60];
    cell *pString;
    REAL fNum;
    int nLen;

    (void)amx;
    /* They should have sent us 1 cell. */
    assert(params[0]/sizeof(cell)==1);

    /* Get the real address of the string. */
    pString=amx_Address(amx,params[1]);

    /* Find out how long the string is in characters. */
    amx_StrLen(pString, &nLen);
    if (nLen == 0 || nLen >= sizeof szSource)
        return 0;

    /* Now convert the Pawn string into a C type null terminated string */
    amx_GetString(szSource, pString, 0, sizeof szSource);

    /* Now convert this to a float. */
    fNum = (REAL)atof(szSource);

    return amx_ftoc(fNum);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatmul(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1
    *   params[2] = float operand 2
    */
    REAL fRes = amx_ctof(params[1]) * amx_ctof(params[2]);
    (void)amx;
    return amx_ftoc(fRes);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatdiv(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float dividend (top)
    *   params[2] = float divisor (bottom)
    */
    REAL fRes = amx_ctof(params[1]) / amx_ctof(params[2]);
    (void)amx;
    return amx_ftoc(fRes);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatadd(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1
    *   params[2] = float operand 2
    */
    REAL fRes = amx_ctof(params[1]) + amx_ctof(params[2]);
    (void)amx;
    return amx_ftoc(fRes);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatsub(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1
    *   params[2] = float operand 2
    */
    REAL fRes = amx_ctof(params[1]) - amx_ctof(params[2]);
    (void)amx;
    return amx_ftoc(fRes);
}

/******************************************************************/
/* Return fractional part of float */
static cell AMX_NATIVE_CALL n_floatfract(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand
    */
    REAL fA = amx_ctof(params[1]);
    fA = fA - (REAL)(floor((double)fA));
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
/* Return integer part of float, rounded */
static cell AMX_NATIVE_CALL n_floatround(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand
    *   params[2] = Type of rounding (integer)
    */
    REAL fA = amx_ctof(params[1]);

    (void)amx;
    switch (params[2])
    {
        case 1:       /* round downwards */
            fA = (REAL)(floor((double)fA));
            break;
        case 2:       /* round upwards */
            fA = (REAL)(ceil((double)fA));
            break;
        case 3:       /* round towards zero (truncate) */
            if ( fA>=0.0 )
                fA = (REAL)(floor((double)fA));
            else
                fA = (REAL)(ceil((double)fA));
            break;
        default:      /* standard, round to nearest */
            fA = (REAL)(floor((double)fA+.5));
            break;
    }

    return (cell)fA;
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatcmp(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1
    *   params[2] = float operand 2
    */
    REAL fA, fB;

    (void)amx;
    fA = amx_ctof(params[1]);
    fB = amx_ctof(params[2]);
    if (fA == fB)
        return 0;
    else if (fA>fB)
        return 1;
    else
        return -1;

}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatsqroot(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand
    */
    REAL fA = amx_ctof(params[1]);
    fA = (REAL)sqrt(fA);
    if (fA < 0)
        return amx_RaiseError(amx, AMX_ERR_DOMAIN);
    return amx_ftoc(fA);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatpower(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1 (base)
    *   params[2] = float operand 2 (exponent)
    */
    REAL fA = amx_ctof(params[1]);
    REAL fB = amx_ctof(params[2]);
    fA = (REAL)pow(fA, fB);
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatlog(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1 (value)
    *   params[2] = float operand 2 (base)
    */
    REAL fValue = amx_ctof(params[1]);
    REAL fBase = amx_ctof(params[2]);
    (void)amx;
    if (fValue <= 0.0 || fBase <= 0)
        return amx_RaiseError(amx, AMX_ERR_DOMAIN);
    if (fBase == 10.0) // ??? epsilon
        fValue = (REAL)log10(fValue);
    else
        fValue = (REAL)(log(fValue) / log(fBase));
    return amx_ftoc(fValue);
}

static REAL ToRadians(REAL angle, int radix)
{
    switch (radix)
    {
        case 1:         /* degrees, sexagesimal system (technically: degrees/minutes/seconds) */
            return (REAL)(angle * PI / 180.0);
        case 2:         /* grades, centesimal system */
            return (REAL)(angle * PI / 200.0);
        default:        /* assume already radian */
            return angle;
    } /* switch */
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatsin(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1 (angle)
    *   params[2] = float operand 2 (radix)
    */
    REAL fA = amx_ctof(params[1]);
    fA = ToRadians(fA, params[2]);
    fA = (float)sin(fA);
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatcos(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1 (angle)
    *   params[2] = float operand 2 (radix)
    */
    REAL fA = amx_ctof(params[1]);
    fA = ToRadians(fA, params[2]);
    fA = (float)cos(fA);
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floattan(AMX *amx,const cell *params)
{
    /*
    *   params[0] = number of bytes
    *   params[1] = float operand 1 (angle)
    *   params[2] = float operand 2 (radix)
    */
    REAL fA = amx_ctof(params[1]);
    fA = ToRadians(fA, params[2]);
    fA = (float)tan(fA);
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
static cell AMX_NATIVE_CALL n_floatabs(AMX *amx,const cell *params)
{
    REAL fA = amx_ctof(params[1]);
    fA = (fA >= 0) ? fA : -fA;
    (void)amx;
    return amx_ftoc(fA);
}

/******************************************************************/
/* return the integer part of a real value, truncated
/* Return integer part of float, truncated (same as floatround
 * with mode 3)
 */
static cell AMX_NATIVE_CALL n_floatint(AMX *amx,const cell *params)
{
    REAL fA = amx_ctof(params[1]);
    if ( fA>=0.0 )
        fA = (REAL)(floor((double)fA));
    else
        fA = (REAL)(ceil((double)fA));
    (void)amx;
    return (cell)fA;
}

#if defined __cplusplus
  extern "C"
#endif
const AMX_NATIVE_INFO float_Natives[] = {
  { "float",       n_float      },
  { "strfloat",    n_strfloat   },
  { "floatmul",    n_floatmul   },
  { "floatdiv",    n_floatdiv   },
  { "floatadd",    n_floatadd   },
  { "floatsub",    n_floatsub   },
  { "floatfract",  n_floatfract },
  { "floatround",  n_floatround },
  { "floatcmp",    n_floatcmp   },
  { "floatsqroot", n_floatsqroot},
  { "floatpower",  n_floatpower },
  { "floatlog",    n_floatlog   },
  { "floatsin",    n_floatsin   },
  { "floatcos",    n_floatcos   },
  { "floattan",    n_floattan   },
  { "floatabs",    n_floatabs   },
  { "floatint",    n_floatint   },  // also add user-defined operator "="
  { NULL, NULL }        /* terminator */
};

int AMXEXPORT AMXAPI amx_FloatInit(AMX *amx)
{
  return amx_Register(amx,float_Natives,-1);
}

int AMXEXPORT AMXAPI amx_FloatCleanup(AMX *amx)
{
  (void)amx;
  return AMX_ERR_NONE;
}
