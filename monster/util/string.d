/*
  Monster - an advanced game scripting language
  Copyright (C) 2004, 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (string.d) is part of the Monster script language
  package.

  Monster is distributed as free software: you can redistribute it
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

module monster.util.string;

import std.string;
import std.utf;

bool begins(char[] str, char[] start)
{
  if(str.length < start.length ||
     str[0..start.length] != start) return false;
  return true;
}

unittest
{
  assert("heia".begins(""));
  assert("heia".begins("h"));
  assert("heia".begins("he"));
  assert(!("heia".begins("H")));
  assert(!("heia".begins("hE")));
  assert("heia".begins("hei"));
  assert("heia".begins("heia"));
  assert(!("heia".begins("heia ")));
  assert(!("heia".begins(" heia")));
  assert(!("heia".begins("eia")));

  assert(!("h".begins("ha")));
  assert(!("h".begins("ah")));
  assert(!("".begins("ah")));
  assert("".begins(""));
}

bool ends(char[] str, char[] end)
{
  if(str.length < end.length ||
     str[$-end.length..$] != end) return false;
  return true;
}

unittest
{
  assert("heia".ends(""));
  assert(!("heia".ends("h")));
  assert("heia".ends("a"));
  assert("heia".ends("ia"));
  assert(!("heia".ends("A")));
  assert(!("heia".ends("Ia")));
  assert("heia".ends("eia"));
  assert("heia".ends("heia"));
  assert(!("heia".ends("heia ")));
  assert(!("heia".ends(" heia")));
  assert(!("heia".ends("hei")));

  assert(!("h".ends("ha")));
  assert(!("h".ends("ah")));
  assert(!("".ends("ah")));
  assert("".ends(""));
}

// Case insensitive version of begins()
bool iBegins(char[] str, char[] start)
{
  if(str.length < start.length ||
     icmp(str[0..start.length], start) != 0) return false;
  return true;
}

unittest
{
  assert("heia".iBegins(""));
  assert("heia".iBegins("H"));
  assert("heia".iBegins("hE"));
  assert("heia".iBegins("hei"));
  assert("HeIa".iBegins("hei"));
  assert("heia".iBegins("heia"));
  assert("hEia".iBegins("heiA"));
  assert(!("heia".iBegins("heia ")));
  assert(!("heIa".iBegins("heia ")));
  assert(!("heia".iBegins("eia")));

  assert(!("h".iBegins("ha")));
  assert(!("h".iBegins("ah")));
  assert(!("".iBegins("ah")));
  assert("".iBegins(""));  
}

// Case insensitive version of begins()
bool iEnds(char[] str, char[] end)
{
  if(str.length < end.length ||
     icmp(str[$-end.length..$], end) != 0) return false;
  return true;
}

unittest
{
  assert("heia".iEnds(""));
  assert(!("heia".iEnds("h")));
  assert("heia".iEnds("a"));
  assert("heia".iEnds("ia"));
  assert("heia".iEnds("A"));
  assert("heia".iEnds("Ia"));
  assert("heia".iEnds("EiA"));
  assert("he ia".iEnds("HE IA"));
  assert("heia".iEnds("eia"));
  assert("heia".iEnds("heia"));
  assert(!("heia".iEnds("heia ")));
  assert(!("heia".iEnds(" heia")));
  assert(!("heia".iEnds("hei")));

  assert(!("h".iEnds("ha")));
  assert(!("h".iEnds("ah")));
  assert(!("".iEnds("ah")));
  assert("".iEnds(""));
}

// A specialized version of std.utf.decode()
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

char[] nextWord(ref char[] str, char delim = ' ')
{
  int i = find(str, delim);
  char[] result;

  // No 'delim' found, return the entire string and set remainder to
  // null.
  if(i == -1)
    {
      result = str;
      str = null;
      return result;
    }

  // A separator was found. Return everything upto 'delim' (index i),
  // put the remainder of the string (not including the char at [i])
  // in str.
  result = str[0..i];
  str = str[i+1..$];
  return result;
}

unittest
{
  char[] test = "bjarne betjent er betent";
  assert(nextWord(test) == "bjarne");
  assert(test == "betjent er betent");
  assert(nextWord(test) == "betjent");
  assert(nextWord(test) == "er");
  assert(test == "betent");
  assert(nextWord(test) == "betent");
  assert(test == "");
  assert(nextWord(test) == "");

  test = ";;foo;bar;";
  assert(nextWord(test,';') == "");
  assert(nextWord(test,';') == "");
  assert(nextWord(test,';') == "foo");
  assert(nextWord(test,';') == "bar");
  assert(nextWord(test,';') == "");
  assert(nextWord(test,';') == "");  
}

// An 'object oriented' interface to nextWord
class NextWord
{
  char delim;
  char[] str;

  this(char[] str, char delim = ' ')
    {
      this.delim = delim;
      this.str = str;
    }

  this(char delim = ' ')
    { this.delim = delim; }

  char[] next()
    { return nextWord(str, delim); }
}

unittest
{
  auto n = new NextWord(";;foo;bar;",';');
  assert(n.next == "");
  assert(n.next == "");
  assert(n.next == "foo");
  assert(n.next == "bar");
  assert(n.next == "");
  assert(n.next == "");  
  n.str = "a;bc";
  assert(n.next == "a");
  assert(n.next == "bc");
}

// Strip trailing zeros
char[] stripz(char [] s)
{
  foreach(int i, char c; s)
    if( c == 0 )
      return s[0..i];

  return s;
}

unittest
{
  assert(stripz(" a b c ") == " a b c ");
  char[8] str;
  str[] = 0;
  assert(stripz(str) == "");
  str[2] = 'o';
  assert(stripz(str) == "");
  str[0] = 'f';
  str[3] = 'd';
  assert(stripz(str) == "f");
  str[1] = 'o';
  assert(stripz(str) == "food");
}

// Convert a long integer into a string using nice comma
// formatting. delim is the delimiter character, size is the number of
// digits in each group. See the unittest for examples.
char[] comma(long i, char delim=',', int size = 3)
{
  char[] str = toString(i);
  char[] res;

  if(i<0) str=str[1..$];

  str.reverse;
  foreach(int j, char c; str)
    { 
      if(j!=0 && j%size == 0)
        res = delim ~ res;
      res = c ~ res;
    }

  if(i<0) res = "-" ~ res;

  return res;
}

unittest
{
  //_________________
  // Inkas       ___ \_
  // were here  /   \  \
  assert(comma(1) == "1");
  assert(comma(12) == "12");
  assert(comma(123) == "123");
  assert(comma(1234) == "1,234");
  assert(comma(12345) == "12,345");
  assert(comma(123456) == "123,456");
  assert(comma(1234567) == "1,234,567");
  assert(comma(12345678) == "12,345,678");

  // Negative values
  assert(comma(-1) == "-1");
  assert(comma(-12) == "-12");
  assert(comma(-123) == "-123");
  assert(comma(-1234) == "-1,234");
  assert(comma(-12345) == "-12,345");

  // Different delimiter
  assert(comma(-888888888888,'-') == "-888-888-888-888");

  // Different size
  assert(comma(1111111111,'.',4) == "11.1111.1111");
}
