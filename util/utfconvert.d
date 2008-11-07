/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (utfconvert.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module util.utfconvert;

import std.utf;

// A specialized version of std.utf.decode(). It returns a bool rather
// than throwing an exception.
private bool fdecode(char[] s, inout size_t idx)
    {
	size_t len = s.length;
	dchar V;
	size_t i = idx;
	char u = s[i];

	if (u & 0x80)
	{   uint n;
	    char u2;

	    /* The following encodings are valid, except for the 5 and 6 byte
	     * combinations:
	     *	0xxxxxxx
	     *	110xxxxx 10xxxxxx
	     *	1110xxxx 10xxxxxx 10xxxxxx
	     *	11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	     *	111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	     *	1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	     */
	    for (n = 1; ; n++)
	    {
		if (n > 4)
		    return false;		// only do the first 4 of 6 encodings
		if (((u << n) & 0x80) == 0)
		{
		    if (n == 1)
			return false;
		    break;
		}
	    }

	    // Pick off (7 - n) significant bits of B from first byte of octet
	    V = cast(dchar)(u & ((1 << (7 - n)) - 1));

	    if (i + (n - 1) >= len)
		return false;			// off end of string

	    /* The following combinations are overlong, and illegal:
	     *	1100000x (10xxxxxx)
	     *	11100000 100xxxxx (10xxxxxx)
	     *	11110000 1000xxxx (10xxxxxx 10xxxxxx)
	     *	11111000 10000xxx (10xxxxxx 10xxxxxx 10xxxxxx)
	     *	11111100 100000xx (10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx)
	     */
	    u2 = s[i + 1];
	    if ((u & 0xFE) == 0xC0 ||
		(u == 0xE0 && (u2 & 0xE0) == 0x80) ||
		(u == 0xF0 && (u2 & 0xF0) == 0x80) ||
		(u == 0xF8 && (u2 & 0xF8) == 0x80) ||
		(u == 0xFC && (u2 & 0xFC) == 0x80))
		return false;			// overlong combination

	    for (uint j = 1; j != n; j++)
	    {
		u = s[i + j];
		if ((u & 0xC0) != 0x80)
		    return false;			// trailing bytes are 10xxxxxx
		V = (V << 6) | (u & 0x3F);
	    }
	    if (!isValidDchar(V))
		return false;
	    i += n;
	}
	else
	{
	    V = cast(dchar) u;
	    i++;
	}

	idx = i;
	return true;
    }

// Converts any string to valid UTF8 so it can be safely printed. It
// does not translate from other encodings but simply replaces invalid
// characters with 'replace'. Does everything in place.
char[] makeUTF8(char[] str, char replace = '?')
{
  size_t idx = 0;
  while(idx < str.length)
    if(!fdecode(str, idx))
      str[idx++] = replace;
  return str;
}
