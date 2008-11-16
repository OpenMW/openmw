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

import std.utf;
import std.string;

// These functions check whether a string begins or ends with a
// certain substring.
bool begins(char[] str, char[] start)
{
  if(str.length < start.length ||
     str[0..start.length] != start) return false;
  return true;
}

bool ends(char[] str, char[] end)
{
  if(str.length < end.length ||
     str[$-end.length..$] != end) return false;
  return true;
}

// Case insensitive versions of begins and ends
bool iBegins(char[] str, char[] start)
{
  if(str.length < start.length ||
     icmp(str[0..start.length], start) != 0) return false;
  return true;
}

bool iEnds(char[] str, char[] end)
{
  if(str.length < end.length ||
     icmp(str[$-end.length..$], end) != 0) return false;
  return true;
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

// Strip trailing zeros
char[] stripz(char [] s)
{
  foreach(int i, char c; s)
    if( c == 0 )
      return s[0..i];

  return s;
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
