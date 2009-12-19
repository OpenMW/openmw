/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (error.d) is part of the Monster script language
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

module monster.vm.error;

import monster.compiler.tokenizer;
version(Tango) import tango.core.Exception;
import std.string;

class MonsterException : Exception
{
  this(char[] msg) { super(/*"MonsterException: " ~*/ msg); }
}

// Source file location
struct Floc
{
  int line = -1;
  char[] fname;

  char[] toString() { return format("%s:%s", fname, line); }
}

void fail(char[] msg, Floc loc)
{
  fail(msg, loc.fname, loc.line);
}

void fail(char[] msg, char[] fname, int line)
{
  if(line != -1)
    fail(format("%s:%s: %s", fname, line, msg));
  else
    fail(msg);
}

void fail(char[] msg)
{
  throw new MonsterException(msg);
}

void fail(char[] msg, TokenArray toks)
{
  if(toks.length)
    fail(msg ~ ", found " ~ toks[0].str, toks[0].loc);
  else
    fail(msg ~ ", found end of file");
}
