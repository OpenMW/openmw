/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (random.d) is part of the Monster script language package.

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

// This module provides simple random number generation. Since this is
// intended for game development, speed and simplicity is favored over
// flexibility and random number quality.
module monster.modules.random;

import monster.monster;
import std.random;

const char[] moduleDef =
"module random;

native uint rand();   // Return a number between 0 and uint.max, inclusive
native float frand(); // Return a number between 0 and 1, inclusive

// Return a random number between a and b, inclusive. Allows negative
// numbers, and works with a>b, a<b and a==b
native int randInt(int a, int b);
"; //"

const float _frandFactor = 1.0/uint.max;

// Return a random integer between a and b, inclusive.
int randInt(int a, int b)
out(result)
{
  // Result must be in range m <= result <= M, where m=min(a,b) and M=max(a,b)
  if(b >= a) assert( (a <= result) && (result <= b) );
  else if(a > b) assert( (b <= result) && (result <= a) );
}
body
{
  if(a>b) return cast(int)(rand() % (a-b+1)) + b;
  else if(b>a) return cast(int)(rand() % (b-a+1)) + a;
  else return a;
}

void initRandomModule()
{
  static MonsterClass mc;
  if(mc !is null) return;

  mc = new MonsterClass(MC.String, moduleDef, "random");

  mc.bind("rand",    { stack.pushInt(rand()); });
  mc.bind("frand",   { stack.pushFloat(rand()*_frandFactor); });
  mc.bind("randInt", { stack.pushInt(randInt(stack.popInt,
                                             stack.popInt)); });
}
