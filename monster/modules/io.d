
// This module provides simple output functions for Monster. The 'i'
// part (input) isn't really there yet.
module monster.modules.io;

import std.stdio;
import monster.monster;

const char[] moduleDef =
"module io;

native write(char[][] args...);
native writeln(char[][] args...);
native writes(char[][] args...);
native writelns(char[][] args...);
native print(char[][] args...);
"; //"

void doWrite(bool space)
{
  AIndex[] args = stack.popAArray();

  char[] form = "%s";
  if(space) form = "%s ";

  foreach(AIndex ind; args)
    writef(form, arrays.getRef(ind).carr);

  fflush(stdout);
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
