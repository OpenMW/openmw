/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (io.d) is part of the Monster script language
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


// This module provides simple output functions for Monster. The 'i'
// (input) part of 'io' isn't really there yet.
module monster.modules.io;

import monster.monster;

const char[] moduleDef =
"module io;

native write(char[][] args...);
native writeln(char[][] args...);
native writes(char[][] args...);
native writelns(char[][] args...);
native print(char[][] args...);
"; //"

// Use tango library functions directly, since flushing the
// minibos-versions will give weird effects when mixing with other
// tango output.
version(Tango)
{
import tango.io.Stdout;
void doWrite(bool space)
{
  AIndex[] args = stack.popAArray();

  char[] form = "{}";
  if(space) form = "{} ";

  foreach(AIndex ind; args)
    Stdout.format(form, arrays.getRef(ind).carr);

  Stdout.flush();
}
void writefln() { Stdout.newline; }
}
else
{ // Phobos
import std.stdio;
void doWrite(bool space)
{
  AIndex[] args = stack.popAArray();

  char[] form = "%s";
  if(space) form = "%s ";

  foreach(AIndex ind; args)
    writef(form, arrays.getRef(ind).carr);

  fflush(stdout);
}
}
void initIOModule()
{
  static MonsterClass mc;
  if(mc !is null) return;

  mc = new MonsterClass(MC.String, moduleDef, "io");

  mc.bind("write",    { doWrite(false);             });
  mc.bind("writeln",  { doWrite(false); writefln(); });
  mc.bind("writes",   { doWrite(true);              });
  mc.bind("writelns", { doWrite(true);  writefln(); });

  // Print is just another name for writelns
  mc.bind("print", { doWrite(true);  writefln(); });
}
