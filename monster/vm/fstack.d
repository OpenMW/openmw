/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (fstack.d) is part of the Monster script language package.

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

module monster.vm.fstack;

import monster.vm.codestream;
import monster.vm.mobject;
import monster.vm.mclass;
import monster.vm.stack;
import monster.vm.error;
import monster.compiler.states;
import monster.compiler.functions;
import monster.compiler.linespec;

// "friendly" parameter and stack handling.
enum SPType
  {
    Function,      // A function (script or native)
    Idle,          // Idle function
    State,         // State code
    NConst,        // Native constructor
  }

// One entry in the function stack
struct StackPoint
{
  CodeStream code; // The byte code handler

  union
  {
    Function *func; // What function we are in (if any)
    State *state; // What state the function belongs to (if any)
  }

  SPType ftype;

  MonsterObject *obj; // "this"-pointer for the function

  // Could have an afterStack to check that the function has the
  // correct imprint (corresponding to an imprint-var in Function.)

  int *frame; // Stack frame, stored when entering the function

  // Get the class owning the function
  MonsterClass getCls()
  {
    assert(isFunc || isState);
    assert(func !is null);
    return func.owner;
  }

  bool isStatic()
  { return isFunc() && func.isStatic; }

  bool isFunc()
  { return (ftype == SPType.Function) || (ftype == SPType.Idle); }

  bool isState()
  { return ftype == SPType.State; }

  // Get the current source position (file name and line
  // number). Mostly used for error messages.
  Floc getFloc()
  {
    assert(isFunc || isState);

    Floc fl;
    fl.fname = getCls().name.loc.fname;

    // Subtract one to make sure we get the last instruction executed,
    // not the next.
    int pos = code.getPos() - 1;
    if(pos < 0) pos = 0;

    fl.line = findLine(func.lines, pos);

    return fl;
  }
}

FunctionStack fstack;

// 30 is somewhat small, but suitable for debugging.
StackPoint fslist[30];

struct FunctionStack
{
  // The current entry
  StackPoint *cur = null;

  // Index of next entry
  int next = 0;

  // Consistancy checks
  invariant()
  {
    assert(next >= 0);
    if(next > 0)
      {
        assert(cur !is null);
        if(cur.ftype == SPType.State)
          assert(next == 1);
      }
    else assert(cur is null);
  }

  // Is the function stack empty?
  bool isEmpty() { return next == 0; }

  // Are we currently running state code?
  bool isStateCode() { return next == 1 && cur.ftype == SPType.State; }

  // Sets up the next stack point and assigns the given object
  private void push(MonsterObject *obj)
  {
    if(next >= fslist.length)
      fail("Function stack overflow - infinite recursion?");

    cur = &fslist[next++];
    cur.obj = obj;

    cur.frame = stack.setFrame();
  }

  // Set the stack point up as a function. Allows obj to be null.
  void push(Function *func, MonsterObject *obj)
  {
    push(obj);
    cur.ftype = SPType.Function;
    cur.func = func;

    assert(obj is null || func.owner.parentOf(obj.cls));

    // Point the code stream to the byte code, if any.
    if(func.isNormal)
      cur.code.setData(func.bcode);

    assert(!func.isIdle, "don't use fstack.push() on idle functions");
  }

  // Set the stack point up as a state
  void push(State *st, MonsterObject *obj)
  {
    assert(st !is null);

    push(obj);
    cur.ftype = SPType.State;
    cur.state = st;

    assert(obj !is null);
    assert(st.owner.parentOf(obj.cls));

    // Set up the byte code
    cur.code.setData(st.bcode);
  }

  // Native constructor
  void pushNConst(MonsterObject *obj)
  {
    assert(obj !is null);
    push(obj);
    cur.ftype = SPType.NConst;
  }

  void pushIdle(Function *fn, MonsterObject *obj)
  {
    push(obj);
    cur.func = fn;
    cur.ftype = SPType.Idle;

    assert(obj is null || fn.owner.parentOf(obj.cls));
    assert(fn.isIdle, fn.name.str ~ "() is not an idle function");
  }

  // Pops one entry of the stack. Checks that the stack level has been
  // returned to the correct position.
  void pop()
  {
    if(next == 0)
      fail("Function stack underflow");

    stack.setFrame(cur.frame);

    if(--next > 0) cur--;
    else cur = null;
  }
}
