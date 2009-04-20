/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (all.d) is part of the Monster script language
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

module monster.modules.all;

import monster.modules.io;
import monster.modules.math;
import monster.modules.timer;
import monster.modules.frames;
import monster.modules.random;
import monster.modules.threads;

import monster.options;

bool has(char[] str, char[] sub)
{
  if(sub.length == 0) return false;

  int diff = str.length;
  int sln = sub.length;

  diff -= sln;
  for(int i=0; i<=diff; i++)
    if(str[i..i+sln] == sub[])
      return true;
  return false;
}

void initAllModules()
{
  static if(moduleList.has("io")) initIOModule();
  static if(moduleList.has("timer")) initTimerModule();
  static if(moduleList.has("frames")) initFramesModule();
  static if(moduleList.has("thread")) initThreadModule();
  static if(moduleList.has("random")) initRandomModule();
  static if(moduleList.has("math")) initMathModule();
}
