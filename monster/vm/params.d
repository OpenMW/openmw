/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (params.d) is part of the Monster script language package.

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

module monster.vm.params;

import monster.vm.mobject;
import monster.vm.thread;

/* This module offers a "friendly" interface for dealing with
   parameters and return values on the stack. It is meant to be an
   alternative to manipulating the stack directly when writing native
   functions.

   NOT FINISHED!
*/

Params params;

struct Params
{
  static:

  // Get the current object (the 'this' reference for the current
  // function)
  MonsterObject *obj()
  {
    assert(cthread !is null);
    assert(cthread.fstack.cur !is null);
    assert(cthread.fstack.cur.obj !is null);
    return cthread.fstack.cur.obj;
  }
}
