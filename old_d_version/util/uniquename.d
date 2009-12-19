/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (uniquename.d) is part of the OpenMW package.

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

module util.uniquename;

import std.stdio;

// Simple auxiliary code for creating unique names in succession,
// eg. "Screenshot_0000000001.png", "Screenshot_0000000002.png", etc..
// WARNING: Uses static storage, so do NOT use persistent references
// or slices to the strings produced here. Intended only for OGRE
// calls, where copies are always made anyway.

struct UniqueName
{
  static this()
  {
    buffer.length = 100;
    number = buffer[0..10];
    number[] = "0000000000";
  }

  static char[] opCall(char[] addon = "")
    {
      // Point p at last digit
      char *p = &number[$-1];
      char *pbeg = number.ptr;
      while(p != pbeg-1)
	{
	  if(*p == '9')
	    {
	      // Carry over
	      *p = '0';
	      p--;
	    }
	  else
	    {
	      // Increase this digit and exit
	      (*p)++;
	      break;
	    }
	}
      assert(p != pbeg-1); // Overflow

      int totLen = 10 + addon.length;
      if(totLen >= buffer.length) totLen = buffer.length - 1;
      if(totLen > 10) buffer[10..totLen] = addon;
      buffer[totLen] = 0; // String must null-terminate

      //writefln("UniqueName result=%s", buffer[0..totLen]);

      return buffer[0..totLen];
    }

 private:
  static char[] buffer;
  static char[] number;
}

unittest
{
  assert(UniqueName() == "0000000001");
  assert(UniqueName() == "0000000002");
  assert(UniqueName() == "0000000003");
  assert(UniqueName() == "0000000004");
  assert(UniqueName() == "0000000005");
  assert(UniqueName() == "0000000006");
  assert(UniqueName("blah") == "0000000007blah");

  assert(UniqueName() == "0000000008");
  assert(UniqueName() == "0000000009");
  assert(UniqueName() == "0000000010");
  assert(UniqueName() == "0000000011");
  assert(UniqueName(" bottles of beer on the wall") ==
	 "0000000012 bottles of beer on the wall");
}
