/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (monster.d) is part of the Monster script language
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

module monster.monster;

public
{
  // These should contain all you need for normal usage.
  import monster.vm.mobject;
  import monster.vm.mclass;
  import monster.vm.stack;
  import monster.vm.vm;
  import monster.vm.thread;
  import monster.vm.idlefunction;
  import monster.vm.arrays;
  import monster.vm.params;
  import monster.vm.error;
}

version(LittleEndian) {}
else static assert(0, "This library does not yet support big endian systems.");
