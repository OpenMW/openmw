/******************************************************************************
* fpattern.h
*	Functions for matching filename patterns to filenames.
*
* Usage
*	Filename patterns are composed of regular (printable) characters which
*	may comprise a filename as well as special pattern matching characters:
*
*	    .		Matches a period (.).
*			Note that a period in a filename is not treated any
*			differently than any other character.
*
*	    ?		Any.
*			Matches any single character except '/' or '\'.
*
*	    *		Closure.
*			Matches zero or more occurences of any characters other
*			than '/' or '\'.
*			Leading '*' characters are allowed.
*
*	    SUB		Substitute (^Z); optionally supported.
*			Similar to '*', this matches zero or more occurences of
*			any characters other than '/', '\', or '.'.
*			Leading '^Z' characters are allowed.
*
*	    [ab]	Set.
*			Matches the single character 'a' or 'b'.
*			If the dash '-' character is to be included, it must
*			immediately follow the opening bracket '['.
*			If the closing bracket ']' character is to be included,
*			it must be preceded by a quote '`'.
*
*	    [a-z]	Range.
*			Matches a single character in the range 'a' to 'z'.
*			Ranges and sets may be combined within the same set of
*			brackets.
*
*	    [!R]	Exclusive range.
*			Matches a single character not in the range 'R'.
*			If range 'R' includes the dash '-' character, the dash
*			must immediately follow the caret '!'.
*
*	    !		Not; optionally supported.
*			Makes the following pattern (up to the next '/') match
*			any filename except those what it would normally match.
*
*	    /		Path separator (UNIX and DOS).
*			Matches a '/' or '\' pathname (directory) separator.
*			Multiple separators are treated like a single
*			separator.
*			A leading separator indicates an absolute pathname.
*
*	    \		Path separator (DOS).
*			Same as the '/' character.
*			Note that this character must be escaped if used within
*			string constants ("\\").
*
*	    \		Quote (UNIX).
*			Makes the next character a regular (nonspecial)
*			character.
*			Note that to match the quote character itself, it must
*			be quoted.
*			Note that this character must be escaped if used within
*			string constants ("\\").
*
*	    `		Quote (DOS).
*			Makes the next character a regular (nonspecial)
*			character.
*			Note that to match the quote character itself, it must
*			be quoted.
*
*	Upper and lower case alphabetic characters are considered identical,
*	i.e., 'a' and 'A' match each other.
*	(What constitutes a lowercase letter depends on the current locale
*	settings.)
*
*	Spaces and control characters are treated as normal characters.
*
* Examples
*	The following patterns in the left column will match the filenames in
*	the middle column and will not match filenames in the right column:
*
*	    Pattern	Will Match			Will Not Match
*	    -------	----------			--------------
*	    a		a (only)			(anything else)
*	    a.		a. (only)			(anything else)
*	    a?c		abc, acc, arc, a.c		a, ac, abbc
*	    a*c		ac, abc, abbc, acc, a.c		a, ab, acb, bac
*	    a*		a, ab, abb, a., a.b		b, ba
*	    *		a, ab, abb, a., .foo, a.foo	(nothing)
*	    *.		a., ab., abb., a.foo.		a, ab, a.foo, .foo
*	    *.*		a., a.b, ah.bc.foo		a
*	    ^Z		a, ab, abb			a., .foo, a.foo
*	    ^Z.		a., ab., abb.			a, .foo, a.foo
*	    ^Z.*	a, a., .foo, a.foo		ab, abb
*	    *2.c	2.c, 12.c, foo2.c, foo.12.c	2x.c
*	    a[b-z]c	abc, acc, azc (only)		(anything else)
*	    [ab0-9]x	ax, bx, 0x, 9x			zx
*	    a[-.]b	a-b, a.b (only)			(anything else)
*	    a[!a-z]b	a0b, a.b, a@b			aab, azb, aa0b
*	    a[!-b]x	a0x, a+x, acx			a-x, abx, axxx
*	    a[-!b]x	a-x, a!x, abx (only)		(anything else)
*	    a[`]]x	a]x (only)			(anything else)
*	    a``x	a`x (only)			(anything else)
*	    oh`!	oh! (only)			(anything else)
*	    is`?it	is?it (only)			(anything else)
*	    !a?c	a, ac, ab, abb, acb, a.foo      abc, a.c, azc
*
* History
*	1.00 1997-01-03 David Tribble.
*		First cut.
*	1.01 1997-01-03 David Tribble.
*		Added '^Z' pattern character.
*		Added fpattern_matchn().
*	1.02 1997-01-26 David Tribble.
*		Changed range negation character from '^' to '!', ala Unix.
*	1.03 1997-08-02 David Tribble.
*		Added 'FPAT_XXX' macro constants.
*
* Limitations
*	This code is copyrighted by the author, but permission is hereby
*	granted for its unlimited use provided that the original copyright
*	and authorship notices are retained intact.
*
*	Other queries can be sent to:
*	    dtribble@technologist.com
*	    david.tribble@beasys.com
*	    dtribble@flash.net
*
* Copyright ©1997 by David R. Tribble, all rights reserved.
*/


#ifndef fpattern_h
#define fpattern_h	1

#ifdef __cplusplus
extern "C"
{
#endif


/* Options */

//#define FPAT_DELIM        /* handle path delimiters in a special way */
//#define FPAT_SUBCLOS      /* support alternative closure */
//#define FPAT_NOT_ENABLED  /* !pattern is enabled */
#define FPAT_MSET_ENABLED   /* multi-set/range is enabled */


/* Manifest constants */

#define FPAT_QUOTE	'`'		/* Quotes a special char	*/
#define FPAT_DEL	'/'		/* Path delimiter (used only when FPAT_DELIM is true) */
#define FPAT_DEL2	'\\'	/* Path delimiter (used only when FPAT_DELIM is true) */
#define FPAT_DOT	'.'		/* Dot char */
#define FPAT_NOT	'!'		/* Exclusion (also used for sets) */
#define FPAT_ANY	'?'		/* Any one char */
#define FPAT_CLOS	'*'		/* Zero or more chars */
#define FPAT_CLOSP	'\x1A'	/* Zero or more nondelimiters (used only when FPAT_SUBCLOS is true) */
#define FPAT_SET_L	'[' 	/* Set/range open bracket	*/
#define FPAT_SET_R	']' 	/* Set/range close bracket	*/
#define FPAT_MSET_L	'{' 	/* Multi-set/range open bracket	*/
#define FPAT_MSET_R	'}' 	/* Multi-set/range close bracket*/
#define FPAT_SET_THRU	'-'	/* Set range of chars		*/


#define FPAT_INVALID	0	/* invalid pattern */
#define FPAT_CLOSED		1	/* valid pattern */
#define FPAT_OPEN		2	/* valid pattern */


/* Public functions */

extern int	fpattern_isvalid(const char *pat);
extern int	fpattern_match(const char *pat, const char *fname, int flength, int keepcase);
extern int	fpattern_matchn(const char *pat, const char *fname, int flength, int keepcase);
extern int	fpattern_matchcount(const char *pat, const char *fname, int flength, int minlength, int keepcase);

#ifdef __cplusplus
}
#endif

#endif /* fpattern_h */
