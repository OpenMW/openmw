/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (linespec.d) is part of the Monster script language
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

/* This module provides a simple system for converting positions in a
   code segment (of compiled byte code) into a line number in source
   code.
*/

module monster.compiler.linespec;

import monster.vm.error;

// A line specification. These are usually found in a list, one for
// each buffer of code. All instructions after position 'pos' belong
// to line 'line', unless a new LineSpec comes after it that covers
// that position.
struct LineSpec
{
  int pos;  // Offset of instruction
  int line; // Line number for instructions after this offset
}

// Find the line belonging to a given position. This does not need to
// be fast, it is only used for error messages and the like.
int findLine(LineSpec[] list, int pos)
{
  int lastpos = -1;
  int lastline = -1;

  assert(pos >= 0);

  // The first entry must represent pos = 0
  if(list.length && list[0].pos != 0)
    fail("Invalid line list: first offset not zero");

  foreach(ls; list)
    {
      if(ls.pos <= lastpos)
        fail("Invalid line list: decreasing offset");

      // Have we searched past pos?
      if(ls.pos > pos)
        // If so, the last entry was the correct one
        return lastline;

      lastpos = ls.pos;
      lastline = ls.line;
    }

  // We never searched past our position, that means the last entry is
  // the most correct.
  return lastline;
}
