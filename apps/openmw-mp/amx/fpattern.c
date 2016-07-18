/*
 from http://home.flash.net/~dtribble/text/fpattern.htm
 2000/12/12
 by Perry
 License statement is in comments below
*/

/******************************************************************************
* fpattern.c
*   Functions for matching filename patterns to filenames.
*
* Usage
*   (See "fpattern.h".)
*
* Notes
*   These pattern matching capabilities are modeled after those found in
*   the UNIX command shells.
*
*   'FPAT_DELIM' must be #define'd if pathname separators are to be handled
*   explicitly.
*
* History
*   1.00 1997-01-03 David Tribble.
*       First cut.
*   1.01 1997-01-03 David Tribble.
*       Added SUB pattern character.
*       Added fpattern_matchn().
*   1.02 1997-01-12 David Tribble.
*       Fixed missing lowercase matching cases.
*   1.03 1997-01-13 David Tribble.
*       Pathname separator code is now controlled by FPAT_DELIM macro.
*   1.04 1997-01-14 David Tribble.
*       Added QUOTE macro.
*   1.05 1997-01-15 David Tribble.
*       Handles special case of empty pattern and empty filename.
*   1.06 1997-01-26 David Tribble.
*       Changed range negation character from '^' to '!', ala Unix.
*   1.07 1997-08-02 David Tribble.
*       Uses the 'FPAT_XXX' constants.
*   1.08 1998-06-28 David Tribble.
*       Minor fixed for MS-VC++ (5.0).
*
*   Modifications by Thiadmer Riemersma (optional case-sensitive matching,
*   optional explicit array length, repeated set syntax), 2005-11-27.
*
* Limitations
*   This code is copyrighted by the author, but permission is hereby
*   granted for its unlimited use provided that the original copyright
*   and authorship notices are retained intact.
*
*   Other queries can be sent to:
*       dtribble@technologist.com
*       david.tribble@beasys.com
*       dtribble@flash.net
*
* Copyright 1997-1998 by David R. Tribble, all rights reserved.
* Copyright (c) 2005-2015, ITB CompuPhase, all rights reserved.
* www.compuphase.com
*
* Version: $Id: fpattern.c 5181 2015-01-21 09:44:28Z thiadmer $
*/



/* System includes */
#if defined TEST
	#include <assert.h>
#elif !defined assert
	#define assert(e)
#endif
#include <string.h>

#include "fpattern.h"


/* Local constants */

#ifndef NULL
# define NULL       ((void *) 0)
#endif

#ifndef FALSE
# define FALSE      0
#endif

#ifndef TRUE
# define TRUE       1
#endif

#ifdef FPAT_SUBCLOS
# if TEST
#   undef           FPAT_CLOSP
#   define FPAT_CLOSP  '~'
# endif
#endif


#define ishexdigit(c)   ( ((c)>='0' && (c)<='9') || ((c)>='a' && (c)<='f') || ((c)>='A' && (c)<='F') )
#define hexdigit(c)     ( ((c)>='0' && (c)<='9') ? (c) - '0' : 10 + \
                        ( ((c)>='a' && (c)<='f') ? (c) - 'a' : (c) - 'A' ) )
#if !defined tolower
# define tolower(c)     ( (c)>='A' && (c)<='Z' ? (c) - 'A' + 'a' : (c) )
#endif

#define BITSET(set,idx) ( (set)[(idx)/8] |= (unsigned char)(1 << ((idx) & 7)) )
#define BITGET(set,idx) ( (set)[(idx)/8] & (1 << ((idx) & 7)) )

/*-----------------------------------------------------------------------------
* fpattern_isvalid()
*   Checks that filename pattern 'pat' is a well-formed pattern.
*
* Returns
*   0 (FPAT_INVALID) if 'pat' is an INVALID pattern, 1 or 2 for VALID patterns
*   (FPAT_CLOSED and FPAT_OPEN respectively).
*   A valid pattern may be "closed" (with a non-repeating terminator) or
*   "open-ended" (where the last character in the pattern may be repeated 0 or
*   more times).
*
* Caveats
*   If 'pat' is NULL, 0 (FPAT_INVALID) is returned.
*
*   If 'pat' is empty (""), 1 (FPAT_CLOSED) is returned, and it is considered a
*   valid (but degenerate) pattern (the only filename it matches is the
*   empty ("") string).
*/

int fpattern_isvalid(const char *pat)
{
    static const char specialchars[] = { FPAT_QUOTE, FPAT_SET_THRU, FPAT_SET_L,
                                         FPAT_SET_R, FPAT_MSET_L, FPAT_MSET_R,
                                         FPAT_ANY, FPAT_NOT, FPAT_CLOS, FPAT_CLOSP,
                                         '\0' };
    int     len, open;
    char    close, pch, pch2;
    unsigned char mask[256/sizeof(unsigned char)];

    /* Check args */
    if (pat == NULL)
        return (FPAT_INVALID);

    open = FALSE;   /* pattern is not open-ended */

    /* Verify that the pattern is valid */
    for (len = 0;  pat[len] != '\0';  len++)
    {
        switch (pat[len])
        {
        case FPAT_SET_L:
#if defined FPAT_MSET_ENABLED
        case FPAT_MSET_L:
#endif
            /* Char set */
            if (pat[len] == FPAT_SET_L)
            {
                open = FALSE;
                close = FPAT_SET_R;
            }
            else
            {
                open = TRUE;
                close = FPAT_MSET_R;
            }
            len++;
            if (pat[len] == FPAT_NOT)
                len++;          /* Set negation */

            memset(mask,0,sizeof mask);
            while (pat[len] != close)
            {
                pch = pat[len];
                if (pat[len] == FPAT_QUOTE) {
                    len++;      /* Quoted char */
                    pch = pat[len];
                    if (ishexdigit(pch)) {
                        len++;  /* Hex digits should come in pairs */
                        if (!ishexdigit(pat[len]))
                            return (FPAT_INVALID);
                        pch2 = pat[len];
                        pch = (hexdigit(pch) << 4) | hexdigit(pch2);
                    } else if (strchr(specialchars,pat[len])==NULL) {
                        return (FPAT_INVALID); /* escaped char should be special char */
                    } /* if */
                } /* if */
                if (pat[len] == '\0')
                    return (FPAT_INVALID); /* Missing closing bracket */
                if (BITGET(mask,pch))
                    return (FPAT_INVALID);
                BITSET(mask,pch);
                len++;

                if (pat[len] == FPAT_SET_THRU && (pat[len+1] != FPAT_SET_THRU || pch == FPAT_SET_THRU))
                {
                    /* Char range */
                    len++;
                    if (pat[len] == FPAT_QUOTE) {
                        len++;      /* Quoted char */
                        if (ishexdigit(pat[len])) {
                            len++;  /* Hex digits should come in pairs */
                            if (!ishexdigit(pat[len]))
                                return (FPAT_INVALID);
                        } else if (strchr(specialchars,pat[len])==NULL) {
                            return (FPAT_INVALID); /* escaped char should be special char */
                        } /* if */
                    } /* if */

                    if (pat[len] == '\0')
                        return (FPAT_INVALID); /* Missing closing bracket */
                    if (len<2 || (unsigned char)pat[len-2]>(unsigned char)pat[len]
                        || (pat[len-2]==pat[len] && pat[len]!=FPAT_SET_THRU))
                        return (FPAT_INVALID);  /* invalid range (decrementing) */
                    //??? also set bits of characters in range?
                    len++;
                }

                if (pat[len] == '\0')
                    return (FPAT_INVALID); /* Missing closing bracket */
            }
            break;

        case FPAT_QUOTE:
            /* Quoted char */
            len++;
            open = FALSE;
            if (ishexdigit(pat[len])) {
                len++;                    /* Hex digits should come in pairs */
                if (!ishexdigit(pat[len]))
                    return (FPAT_INVALID);
            } else if (strchr(specialchars,pat[len])==NULL) {
                return (FPAT_INVALID);    /* escaped char should be special char */
            } /* if */
            if (pat[len] == '\0')
                return (FPAT_INVALID);    /* Missing quoted char */
            break;

#if defined FPAT_NOT_ENABLED
        case FPAT_NOT:
            /* Negated pattern */
            len++;
            open = FALSE;
            if (pat[len] == '\0')
                return (FPAT_INVALID);     /* Missing subpattern */
            break;
#endif

        case FPAT_CLOS:
            open = TRUE;
            break;

        default:
            /* Valid character */
            open = FALSE;
            break;
        }
    }

    return (open ? FPAT_OPEN : FPAT_CLOSED);
}


/* NOTA BENE
 * =========
 * To modify the pattern matcher, modify the case-insensitive version and
 * copy it to the case-sensitive version. In steps:
 * - modify the function fpatter_submatch_tolower(), here below
 * - make a copy of the entire function
 * - erase all references to "tolower" in the copy
 */

/*-----------------------------------------------------------------------------
* fpattern_submatch_tolower()
*   Attempts to match subpattern 'pat' to subfilename 'fname'.
*
* Returns
*   1 (true) if the subfilename matches, otherwise 0 (false).
*
* Caveats
*   This does not assume that 'pat' is well-formed.
*
*   If 'pat' is empty (""), the only filename it matches is the empty ("")
*   string.
*
*   Some non-empty patterns (e.g., "") will match an empty filename ("").
*/

static int fpattern_submatch_tolower(const char *pat, const char *fname, int flength)
{
    int     fch;
    int     pch, pch2;
    int     i;
    int     yes, match;
    int     lo, hi;

    /* Attempt to match subpattern against subfilename */
    while (*pat != '\0')
    {
        fch = (unsigned char)*fname;
        pch = (unsigned char)*pat;
        pat++;

        switch (pch)
        {
        case FPAT_ANY:
            /* Match a single char */
        #if defined FPAT_DELIM
            if (fch == FPAT_DEL  ||  fch == FPAT_DEL2  ||  fch == '\0')
                return (FALSE);
        #else
            if (flength == 0)
                return (FALSE);
        #endif
            fname++;
            flength--;
            break;

        case FPAT_CLOS:
            /* Match zero or more chars */
        #if defined FPAT_DELIM
            i = 0;
            while (fname[i] != '\0'  &&
                    fname[i] != FPAT_DEL  &&  fname[i] != FPAT_DEL2)
                i++;
        #else
            i = flength;
        #endif
            while (i >= 0)
            {
                if (fpattern_submatch_tolower(pat, fname+i, flength-i))
                    return (TRUE);
                i--;
            }
            return (FALSE);

    #ifdef FPAT_SUBCLOS
        case FPAT_CLOSP:
            /* Match zero or more chars */
            i = 0;
            while (i < flength  &&
        #if defined FPAT_DELIM
                    fname[i] != FPAT_DEL  &&  fname[i] != FPAT_DEL2  &&
        #endif
                    fname[i] != '.')
                i++;
            while (i >= 0)
            {
                if (fpattern_submatch_tolower(pat, fname+i, flength-i))
                    return (TRUE);
                i--;
            }
            return (FALSE);
    #endif

        case FPAT_QUOTE:
            /* Match a quoted char */
            assert(*pat != '\0');
            pch = (unsigned char)*pat;
            if (ishexdigit(pch)) {
                pch2 = *++pat;
                pch = (hexdigit(pch) << 4) | hexdigit(pch2);
            } /* if */
            if (tolower(fch) != tolower(pch))
                return (FALSE);
            fname++;
            flength--;
            pat++;
            break;

        case FPAT_SET_L:
            /* Match char set/range */
            yes = TRUE;
            if (*pat == FPAT_NOT) {
               pat++;
               yes = FALSE; /* Set negation */
            } /* if */

            /* Look for [s], [-], [abc], [a-c] */
            match = !yes;
            while (*pat != FPAT_SET_R) {
                assert(*pat != '\0');
                pch = (unsigned char)*pat++;
                if (pch == FPAT_QUOTE) {
                    assert(*pat != '\0');
                    pch = (unsigned char)*pat++;
                    if (ishexdigit(pch)) {
                        pch2 = *pat++;
                        lo = (hexdigit(pch) << 4) | hexdigit(pch2);
                    } else {
                        lo = pch;
                    } /* if */
                } else {
                    lo = pch;
                } /* if */

                if (*pat == FPAT_SET_THRU) {
                    /* Range */
                    pat++;
                    pch = (unsigned char)*pat++;

                    if (pch == FPAT_QUOTE) {
                        assert(*pat != '\0');
                        pch = (unsigned char)*pat++;
                        if (ishexdigit(pch)) {
                            pch2 = *pat++;
                            hi = (hexdigit(pch) << 4) | hexdigit(pch2);
                        } else {
                            hi = pch;
                        } /* if */
                    } else {
                      hi = pch;
                    } /* if */

                    /* Compare character to set range */
                    if (tolower(fch) >= tolower(lo) && tolower(fch) <= tolower(hi)) {
                        match = yes;
                        /* skip to the end of the set in the pattern (no need to
                         * search further once a match is found)
                         */
                        while (*pat != FPAT_SET_R) {
                            assert(*pat != '\0');
                            pat++;
                        } /* while */
                        break;
                    } /* if */
                } else {
                    /* Compare character to single char from set */
                    if (tolower(fch) == tolower(lo)) {
                        match = yes;
                        /* skip to the end of the set in the pattern (no need to
                         * search further once a match is found)
                         */
                        while (*pat != FPAT_SET_R) {
                            assert(*pat != '\0');
                            pat++;
                        } /* while */
                        break;
                    } /* if */
                } /* if */

                assert(*pat != '\0');
            } /* while */

            if (!match)
                return (FALSE);

            fname++;
            flength--;
            assert(*pat == FPAT_SET_R);
            pat++;
            break;

#if defined FPAT_MSET_ENABLED
        case FPAT_MSET_L:
            /* Match zero or more characters in a char set/range */
            yes = TRUE;
            if (*pat == FPAT_NOT) {
               pat++;
               yes = FALSE; /* Set negation */
            } /* if */

            do {
                const char *org_pat = pat;
                /* Look for [s], [-], [abc], [a-c] */
                match = !yes;
                while (*pat != FPAT_MSET_R) {
                    assert(*pat != '\0');
                    pch = (unsigned char)*pat++;
                    if (pch == FPAT_QUOTE) {
                        assert(*pat != '\0');
                        pch = (unsigned char)*pat++;
                        if (ishexdigit(pch)) {
                            pch2 = *pat++;
                            lo = (hexdigit(pch) << 4) | hexdigit(pch2);
                        } else {
                            lo = pch;
                        } /* if */
                    } else {
                        lo = pch;
                    } /* if */

                    if (*pat == FPAT_SET_THRU) {
                        /* Range */
                        pat++;
                        pch = (unsigned char)*pat++;

                        if (pch == FPAT_QUOTE) {
                            assert(*pat != '\0');
                            pch = (unsigned char)*pat++;
                            if (ishexdigit(pch)) {
                                pch2 = *pat++;
                                hi = (hexdigit(pch) << 4) | hexdigit(pch2);
                            } else {
                                hi = pch;
                            } /* if */
                        } else {
                            hi = pch;
                        } /* if */

                        /* Compare character to set range */
                        if (tolower(fch) >= tolower(lo) && tolower(fch) <= tolower(hi)) {
                            match = yes;
                            /* skip to the end of the set in the pattern (no
                             * need to search further once a match is found)
                             */
                            while (*pat != FPAT_MSET_R) {
                                assert(*pat != '\0');
                                pat++;
                            } /* while */
                            break;
                        } /* if */
                    } else {
                        /* Compare character to single char from the set */
                        if (tolower(fch) == tolower(lo)) {
                            match = yes;
                            /* skip to the end of the set in the pattern (no
                             * need to search further once a match is found)
                             */
                            while (*pat != FPAT_MSET_R) {
                                assert(*pat != '\0');
                                pat++;
                            } /* while */
                            break;
                        } /* if */
                    } /* if */

                    assert(*pat != '\0');
                } /* while */

                assert(*pat == FPAT_MSET_R);

                if (match) {
                    fname++;
                    flength--;
                    fch = *fname;
                    if (flength > 0)
                        pat = org_pat;
                } /* if */

            } while (match && flength > 0);

            pat++;
            break;
#endif /* FPAT_MSET_ENABLED */

#if defined FPAT_NOT_ENABLED
        case FPAT_NOT:
            /* Match only if rest of pattern does not match */
            assert(*pat != '\0');
            i = fpattern_submatch_tolower(pat, fname, flength);
            return !i;
#endif

#if defined FPAT_DELIM
        case FPAT_DEL:
    #if FPAT_DEL2 != FPAT_DEL
        case FPAT_DEL2:
    #endif
            /* Match path delimiter char */
            if (fch != FPAT_DEL  &&  fch != FPAT_DEL2)
                return (FALSE);
            fname++;
            flength--;
            break;
#endif

        default:
            /* Match a (non-null) char exactly */
            if (tolower(fch) != tolower(pch))
                return (FALSE);
            fname++;
            flength--;
            break;
        }
    }

    /* Check for complete match */
    if (flength != 0)
        return (FALSE);

    /* Successful match */
    return (TRUE);
}

/*
* fpattern_submatch()
*   See fpattern_submatch_tolower()
*/

static int fpattern_submatch(const char *pat, const char *fname, int flength)
{
    int     fch;
    int     pch, pch2;
    int     i;
    int     yes, match;
    int     lo, hi;

    /* Attempt to match subpattern against subfilename */
    while (*pat != '\0')
    {
        fch = (unsigned char)*fname;
        pch = (unsigned char)*pat;
        pat++;

        switch (pch)
        {
        case FPAT_ANY:
            /* Match a single char */
        #if defined FPAT_DELIM
            if (fch == FPAT_DEL  ||  fch == FPAT_DEL2  ||  fch == '\0')
                return (FALSE);
        #else
            if (flength == 0)
                return (FALSE);
        #endif
            fname++;
            flength--;
            break;

        case FPAT_CLOS:
            /* Match zero or more chars */
        #if defined FPAT_DELIM
            i = 0;
            while (fname[i] != '\0'  &&
                    fname[i] != FPAT_DEL  &&  fname[i] != FPAT_DEL2)
                i++;
        #else
            i = flength;
        #endif
            while (i >= 0)
            {
                if (fpattern_submatch(pat, fname+i, flength-i))
                    return (TRUE);
                i--;
            }
            return (FALSE);

    #ifdef FPAT_SUBCLOS
        case FPAT_CLOSP:
            /* Match zero or more chars */
            i = 0;
            while (i < flength  &&
        #if defined FPAT_DELIM
                    fname[i] != FPAT_DEL  &&  fname[i] != FPAT_DEL2  &&
        #endif
                    fname[i] != '.')
                i++;
            while (i >= 0)
            {
                if (fpattern_submatch(pat, fname+i, flength-i))
                    return (TRUE);
                i--;
            }
            return (FALSE);
    #endif

        case FPAT_QUOTE:
            /* Match a quoted char */
            assert(*pat != '\0');
            pch = (unsigned char)*pat;
            if (ishexdigit(pch)) {
                pch2 = *++pat;
                pch = (hexdigit(pch) << 4) | hexdigit(pch2);
            } /* if */
            if ((fch) != (pch))
                return (FALSE);
            fname++;
            flength--;
            pat++;
            break;

        case FPAT_SET_L:
            /* Match char set/range */
            yes = TRUE;
            if (*pat == FPAT_NOT) {
               pat++;
               yes = FALSE; /* Set negation */
            } /* if */

            /* Look for [s], [-], [abc], [a-c] */
            match = !yes;
            while (*pat != FPAT_SET_R) {
                assert(*pat != '\0');
                pch = (unsigned char)*pat++;
                if (pch == FPAT_QUOTE) {
                    assert(*pat != '\0');
                    pch = (unsigned char)*pat++;
                    if (ishexdigit(pch)) {
                        pch2 = *pat++;
                        lo = (hexdigit(pch) << 4) | hexdigit(pch2);
                    } else {
                        lo = pch;
                    } /* if */
                } else {
                    lo = pch;
                } /* if */

                if (*pat == FPAT_SET_THRU) {
                    /* Range */
                    pat++;
                    pch = (unsigned char)*pat++;

                    if (pch == FPAT_QUOTE) {
                        assert(*pat != '\0');
                        pch = (unsigned char)*pat++;
                        if (ishexdigit(pch)) {
                            pch2 = *pat++;
                            hi = (hexdigit(pch) << 4) | hexdigit(pch2);
                        } else {
                            hi = pch;
                        } /* if */
                    } else {
                      hi = pch;
                    } /* if */

                    /* Compare character to set range */
                    if ((fch) >= (lo) && (fch) <= (hi)) {
                        match = yes;
                        /* skip to the end of the set in the pattern (no need to
                         * search further once a match is found)
                         */
                        while (*pat != FPAT_SET_R) {
                            assert(*pat != '\0');
                            pat++;
                        } /* while */
                        break;
                    } /* if */
                } else {
                    /* Compare character to single char from set */
                    if ((fch) == (lo)) {
                        match = yes;
                        /* skip to the end of the set in the pattern (no need to
                         * search further once a match is found)
                         */
                        while (*pat != FPAT_SET_R) {
                            assert(*pat != '\0');
                            pat++;
                        } /* while */
                        break;
                    } /* if */
                } /* if */

                assert(*pat != '\0');
            } /* while */

            if (!match)
                return (FALSE);

            fname++;
            flength--;
            assert(*pat == FPAT_SET_R);
            pat++;
            break;

#if defined FPAT_MSET_ENABLED
        case FPAT_MSET_L:
            /* Match zero or more characters in a char set/range */
            yes = TRUE;
            if (*pat == FPAT_NOT) {
               pat++;
               yes = FALSE; /* Set negation */
            } /* if */

            do {
                const char *org_pat = pat;
                /* Look for [s], [-], [abc], [a-c] */
                match = !yes;
                while (*pat != FPAT_MSET_R) {
                    assert(*pat != '\0');
                    pch = (unsigned char)*pat++;
                    if (pch == FPAT_QUOTE) {
                        assert(*pat != '\0');
                        pch = (unsigned char)*pat++;
                        if (ishexdigit(pch)) {
                            pch2 = *pat++;
                            lo = (hexdigit(pch) << 4) | hexdigit(pch2);
                        } else {
                            lo = pch;
                        } /* if */
                    } else {
                        lo = pch;
                    } /* if */

                    if (*pat == FPAT_SET_THRU) {
                        /* Range */
                        pat++;
                        pch = (unsigned char)*pat++;

                        if (pch == FPAT_QUOTE) {
                            assert(*pat != '\0');
                            pch = (unsigned char)*pat++;
                            if (ishexdigit(pch)) {
                                pch2 = *pat++;
                                hi = (hexdigit(pch) << 4) | hexdigit(pch2);
                            } else {
                                hi = pch;
                            } /* if */
                        } else {
                            hi = pch;
                        } /* if */

                        /* Compare character to set range */
                        if ((fch) >= (lo) && (fch) <= (hi)) {
                            match = yes;
                            /* skip to the end of the set in the pattern (no
                             * need to search further once a match is found)
                             */
                            while (*pat != FPAT_MSET_R) {
                                assert(*pat != '\0');
                                pat++;
                            } /* while */
                            break;
                        } /* if */
                    } else {
                        /* Compare character to single char from the set */
                        if ((fch) == (lo)) {
                            match = yes;
                            /* skip to the end of the set in the pattern (no
                             * need to search further once a match is found)
                             */
                            while (*pat != FPAT_MSET_R) {
                                assert(*pat != '\0');
                                pat++;
                            } /* while */
                            break;
                        } /* if */
                    } /* if */

                    assert(*pat != '\0');
                } /* while */

                assert(*pat == FPAT_MSET_R);

                if (match) {
                    fname++;
                    flength--;
                    fch = *fname;
                    if (flength > 0)
                        pat = org_pat;
                } /* if */

            } while (match && flength > 0);

            pat++;
            break;
#endif  /* FPAT_MSET_ENABLED */

#if defined FPAT_NOT_ENABLED
        case FPAT_NOT:
            /* Match only if rest of pattern does not match */
            assert(*pat != '\0');
            i = fpattern_submatch(pat, fname, flength);
            return !i;
#endif

#if defined FPAT_DELIM
        case FPAT_DEL:
    #if FPAT_DEL2 != FPAT_DEL
        case FPAT_DEL2:
    #endif
            /* Match path delimiter char */
            if (fch != FPAT_DEL  &&  fch != FPAT_DEL2)
                return (FALSE);
            fname++;
            flength--;
            break;
#endif

        default:
            /* Match a (non-null) char exactly */
            if ((fch) != (pch))
                return (FALSE);
            fname++;
            flength--;
            break;
        }
    }

    /* Check for complete match */
    if (flength != 0)
        return (FALSE);

    /* Successful match */
    return (TRUE);
}


/*-----------------------------------------------------------------------------
* fpattern_match()
*   Attempts to match pattern 'pat' to filename 'fname'. The comparison is case
* sensitive if 'keepcase' is true, and case insensitive otherwise. The 'flength'
* parameter allows to check partial strings, or to check strings with embedded
* zero bytes. When 'flength' is -1, it is set to the string length.
*
* Returns
*   1 (true) if the filename matches, otherwise 0 (false).
*
* Caveats
*   If 'fname' is null, zero (false) is returned.
*
*   If 'pat' is null, zero (false) is returned.
*
*   If 'pat' is empty (""), the only filename it matches is the empty
*   string ("").
*
*   If 'fname' is empty, the only pattern that will match it is the empty
*   string ("").
*
*   If 'pat' is not a well-formed pattern, zero (false) is returned.
*
*   Upper and lower case letters are treated the same; alphabetic
*   characters are converted to lower case before matching occurs.
*   Conversion to lower case is dependent upon the current locale setting.
*/

int fpattern_match(const char *pat, const char *fname, int flength, int keepcase)
{
    int     rc;

    /* Check args */
    if (fname == NULL)
        return (FALSE);

    if (pat == NULL)
        return (FALSE);

    /* Verify that the pattern is valid, and get its length */
    if (!fpattern_isvalid(pat))
        return (FALSE);

    /* Attempt to match pattern against filename */
    if (flength < 0)
        flength = (int)strlen(fname);
    if (flength == 0)
        return (pat[0] == '\0');    /* Special case */
    if (keepcase)
        rc = fpattern_submatch(pat, fname, flength);
    else
        rc = fpattern_submatch_tolower(pat, fname, flength);

    return (rc);
}


/*-----------------------------------------------------------------------------
* fpattern_matchn()
*   Attempts to match pattern 'pat' to filename 'fname'.
*   This operates like fpattern_match() except that it does not verify that
*   pattern 'pat' is well-formed, assuming that it has been checked by a
*   prior call to fpattern_isvalid().
*
*   NOTA BENE: this function may crash on invalid patterns!
*   =======================================================
*
* Returns
*   1 (true) if the filename matches, otherwise 0 (false).
*
* Caveats
*   If 'fname' is null, zero (false) is returned.
*
*   If 'pat' is null, zero (false) is returned.
*
*   If 'pat' is empty (""), the only filename it matches is the empty ("")
*   string.
*
*   If 'pat' is not a well-formed pattern, unpredictable results may occur.
*
*   Upper and lower case letters are treated the same; alphabetic
*   characters are converted to lower case before matching occurs.
*   Conversion to lower case is dependent upon the current locale setting.
*
* See also
*   fpattern_match().
*/

int fpattern_matchn(const char *pat, const char *fname, int flength, int keepcase)
{
    int     rc;

    /* Check args */
    if (fname == NULL)
        return (FALSE);

    if (pat == NULL)
        return (FALSE);

    /* Assume that pattern is well-formed
     * NOTA BENE: this function may crash on invalid patterns!
     */
    assert(fpattern_isvalid(pat));

    /* Attempt to match pattern against filename */
    if (flength < 0)
        flength = (int)strlen(fname);
    if (keepcase)
        rc = fpattern_submatch(pat, fname, flength);
    else
        rc = fpattern_submatch_tolower(pat, fname, flength);

    return (rc);
}

/* returns the largest packet that matches the pattern */
int fpattern_matchcount(const char *pat, const char *fname, int flength, int minlength, int keepcase)
{
    int len;

    if (fname == NULL)
        return (FALSE);

    if (pat == NULL)
        return (FALSE);

    /* Assume that pattern is well-formed */

    /* Attempt to match pattern against filename */
    if (flength < 0)
        flength = (int)strlen(fname);

    if (keepcase) {
        for (len = flength; len > minlength; len--)
            if (fpattern_submatch(pat, fname, len))
                break;
    } else {
        for (len = flength; len > minlength; len--)
            if (fpattern_submatch_tolower(pat, fname, len))
                break;
    } /* if */

    return (len > minlength) ? len : 0;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#ifdef TEST

#include <locale.h>
#include <stdio.h>
#if defined _WIN32 || defined __WIN32__ || defined WIN32
# include <stdlib.h>
# define sleep(q) Sleep(q*1000)
# include <windows.h>
#endif

/* Local variables */

static int  count = 0;
static int  fails = 0;
static int  stop_on_fail = FALSE;


/*-----------------------------------------------------------------------------
* test()
*/

static void test(int expect, const char *fname, const char *pat)
{
    int     failed;
    int     result;
    char    fbuf[80+1];
    char    pbuf[80+1];

    count++;
    printf("%3d. ", count);

    if (fname == NULL)
    {
        printf("<null>\n");
    }
    else
    {
        strcpy(fbuf, fname);
        printf("\"%s\"\n", fbuf);
    }

    if (pat == NULL)
    {
        printf("     <null>\n");
    }
    else
    {
        strcpy(pbuf, pat);
        printf("     \"%s\"\n", pbuf);
    }

    result = fpattern_match(pat == NULL ? NULL : pbuf,
                            fname == NULL ? NULL : fbuf,
                            -1, FALSE);

    failed = (result != expect);
    printf("    -> %c, expected %c: %s\n",
        "FT"[!!result], "FT"[!!expect], failed ? "FAIL ***" : "pass");

    if (failed)
    {
        fails++;

        if (stop_on_fail)
            exit(1);
        sleep(1);
    }

    printf("\n");
}


/*-----------------------------------------------------------------------------
* main()
*   Test driver.
*/

int main(int argc, char **argv)
{
    (void) argc;    /* Shut up lint */
    (void) argv;    /* Shut up lint */

#if DEBUG
    dbg_f = stdout;
#endif

    printf("==========================================\n");

    setlocale(LC_CTYPE, "");

#if 1   /* Set to nonzero to stop on first failure */
    stop_on_fail = TRUE;
#endif

    test(0, NULL,   NULL);
    test(0, NULL,   "");
    test(0, NULL,   "abc");
    test(0, "", NULL);
    test(0, "abc",  NULL);

    test(1, "abc",      "abc");
    test(0, "ab",       "abc");
    test(0, "abcd",     "abc");
    test(0, "Foo.txt",  "Foo.x");
    test(1, "Foo.txt",  "Foo.txt");
    test(1, "Foo.txt",  "foo.txt");
    test(1, "FOO.txt",  "foo.TXT");

    test(1, "a",        "?");
    test(1, "foo.txt",  "f??.txt");
    test(1, "foo.txt",  "???????");
    test(0, "foo.txt",  "??????");
    test(0, "foo.txt",  "????????");

    test(1, "*",        "`*");
    test(1, "A[",       "a`[");
    test(1, "a`x",      "a``x");
    test(1, "*`?",      "`*```?");
    test(1, "a*x",      "a`*x");
    test(1, "a€",       "a`80");
    test(0, "a€",       "a`8");

#if defined FPAT_DELIM
    test(0, "",         "/");
    test(0, "",         "\\");
    test(1, "/",        "/");
    test(1, "/",        "\\");
    test(1, "\\",       "/");
    test(1, "\\",       "\\");

    test(1, "a/b",      "a/b");
    test(1, "a/b",      "a\\b");

    test(1, "/",        "*/*");
    test(1, "foo/a.c",  "f*/*.?");
    test(1, "foo/a.c",  "*/*");
    test(0, "foo/a.c",  "/*/*");
    test(0, "foo/a.c",  "*/*/");

    test(1, "/",        "~/~");
    test(1, "foo/a.c",  "f~/~.?");
    test(0, "foo/a.c",  "~/~");
    test(1, "foo/abc",  "~/~");
    test(0, "foo/a.c",  "/~/~");
    test(0, "foo/a.c",  "~/~/");
#endif

    test(0, "",         "*");
    test(1, "a",        "*");
    test(1, "ab",       "*");
    test(1, "abc",      "**");
    test(1, "ab.c",     "*.?");
    test(1, "ab.c",     "*.*");
    test(1, "ab.c",     "*?");
    test(1, "ab.c",     "?*");
    test(1, "ab.c",     "?*?");
    test(1, "ab.c",     "?*?*");
    test(1, "ac",       "a*c");
    test(1, "axc",      "a*c");
    test(1, "ax-yyy.c", "a*c");
    test(1, "ax-yyy.c", "a*x-yyy.c");
    test(1, "axx/yyy.c",    "a*x/*c");

#ifdef FPAT_SUBCLOS
    test(0, "",         "~");
    test(1, "a",        "~");
    test(1, "ab",       "~");
    test(1, "abc",      "~~");
    test(1, "ab.c",     "~.?");
    test(1, "ab.c",     "~.~");
    test(0, "ab.c",     "~?");
    test(0, "ab.c",     "?~");
    test(0, "ab.c",     "?~?");
    test(1, "ab.c",     "?~.?");
    test(1, "ab.c",     "?~?~");
    test(1, "ac",       "a~c");
    test(1, "axc",      "a~c");
    test(0, "ax-yyy.c", "a~c");
    test(1, "ax-yyyvc", "a~c");
    test(1, "ax-yyy.c", "a~x-yyy.c");
    test(0, "axx/yyy.c","a~x/~c");
    test(1, "axx/yyyvc","a~x/~c");
#endif

#if defined FPAT_NOT_ENABLED
    test(0, "a",        "!");
    test(0, "a",        "!a");
    test(1, "a",        "!b");
    test(1, "abc",      "!abb");
    test(0, "a",        "!*");
    test(1, "abc",      "!*.?");
    test(1, "abc",      "!*.*");
    test(0, "",         "!*");      /*!*/
    test(0, "",         "!*?");     /*!*/
    test(0, "a",        "!*?");
    test(0, "a",        "a!*");
    test(1, "a",        "a!?");
    test(1, "a",        "a!*?");
    test(1, "ab",       "*!?");
    test(1, "abc",      "*!?");
    test(0, "ab",       "?!?");
    test(1, "abc",      "?!?");
    test(0, "a-b",      "!a[-]b");
    test(0, "a-b",      "!a[x-]b");
    test(0, "a=b",      "!a[x-]b");
    test(0, "a-b",      "!a[x`-]b");
    test(1, "a=b",      "!a[x`-]b");
    test(0, "a-b",      "!a[x---]b");
    test(1, "a=b",      "!a[x---]b");
#endif

    test(1, "abc",      "a[b]c");
    test(1, "aBc",      "a[b]c");
    test(1, "abc",      "a[bB]c");
    test(1, "abc",      "a[bcz]c");
    test(1, "azc",      "a[bcz]c");
    test(0, "ab",       "a[b]c");
    test(0, "ac",       "a[b]c");
    test(0, "axc",      "a[b]c");

    test(0, "abc",      "a[!b]c");
    test(0, "abc",      "a[!bcz]c");
    test(0, "azc",      "a[!bcz]c");
    test(0, "ab",       "a[!b]c");
    test(0, "ac",       "a[!b]c");
    test(1, "axc",      "a[!b]c");
    test(1, "axc",      "a[!bcz]c");

    test(1, "a1z",      "a[0-9]z");
    test(0, "a1",       "a[0-9]z");
    test(0, "az",       "a[0-9]z");
    test(0, "axz",      "a[0-9]z");
    test(1, "a2z",      "a[-0-9]z");
    test(1, "a-z",      "a[-0-9]z");
    test(1, "a-b",      "a[-]b");
    test(0, "a-b",      "a[x-]b");
    test(0, "a=b",      "a[x-]b");
    test(1, "a-b",      "a[x`-]b");
    test(0, "a=b",      "a[x`-]b");
    test(1, "a-b",      "a[x---]b");
    test(0, "a=b",      "a[x---]b");

    test(0, "a0z",      "a[!0-9]z");
    test(1, "aoz",      "a[!0-9]z");
    test(0, "a1",       "a[!0-9]z");
    test(0, "az",       "a[!0-9]z");
    test(0, "a9Z",      "a[!0-9]z");
    test(1, "acz",      "a[!-0-9]z");
    test(0, "a7z",      "a[!-0-9]z");
    test(0, "a-z",      "a[!-0-9]z");
    test(0, "a-b",      "a[!-]b");
    test(0, "a-b",      "a[!x-]b");
    test(0, "a=b",      "a[!x-]b");
    test(0, "a-b",      "a[!x`-]b");
    test(1, "a=b",      "a[!x`-]b");
    test(0, "a-b",      "a[!x---]b");
    test(1, "a=b",      "a[!x---]b");

    test(1, "a!z",      "a[`!0-9]z");
    test(1, "a3Z",      "a[`!0-9]z");
    test(0, "A3Z",      "a[`!0`-9]z");
    test(1, "a9z",      "a[`!0`-9]z");
    test(1, "a-z",      "a[`!0`-9]z");

    test(1, "ac",       "a{b}c");
    test(1, "abc",      "a{b}c");
    test(1, "abbc",     "a{b}c");
    test(1, "aBbBc",    "a{b}c");
    test(1, "abc",      "a{bB}c");
    test(1, "abc",      "a{bpz}c");
    test(1, "azc",      "a{bcz}");
    test(0, "ab",       "a{b}c");
    test(0, "axc",      "a{b}c");

    assert(fpattern_isvalid("a[`[`]]") == 1);
    assert(fpattern_isvalid("a{`{`}}") == 2);
    assert(fpattern_isvalid("a[b-z]") == 1);
    assert(fpattern_isvalid("a?") == 1);
    assert(fpattern_isvalid("a{b-z}") == 2);
    assert(fpattern_isvalid("a*") == 2);
    assert(fpattern_isvalid("a[aba]") == FPAT_INVALID);
    assert(fpattern_isvalid("a[z-b]") == FPAT_INVALID);
    assert(fpattern_isvalid("a[b c ]") == FPAT_INVALID);
    assert(fpattern_isvalid("a[`a]") == FPAT_INVALID);

    assert(fpattern_isvalid("[`80-`ff]") == FPAT_CLOSED);
    assert(fpattern_isvalid("[`80-`ff]{`00-`7f}") == FPAT_OPEN);
    assert(fpattern_isvalid("???") == FPAT_CLOSED);
    test(1, "\x90\x40\x4f", "[`80-`ff]{`00-`7f}");
    test(0, "\x90\x40\x4f\x90", "[`80-`ff]{`00-`7f}");
    test(1, "\x90\x40\x4f", "[`80`90]*");

    printf("%d tests, %d failures\n", count, fails);
    return (fails == 0 ? 0 : 1);
}


#endif /* TEST */

/* End fpattern.c */
