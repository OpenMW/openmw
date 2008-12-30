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

// "friendly" parameter and stack handling.
enum SPType
  {
    Function,      // A function (script or native)
    State,         // State code
    NConst,        // Native constructor

    // The idle function callbacks are split because they handle the
    // stack differently.
    Idle_Initiate, // IdleFunction.initiate()
    Idle_Reentry,  // IdleFunction.reentry()
    Idle_Abort,    // IdleFunction.abort()
    Idle_Check     // IdleFunction.hasFinished()
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
  MonsterClass cls; // class owning the function

  int afterStack; // Where the stack should be when this function
                  // returns
  int *frame; // Stack frame, stored when entering the function
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
    cur.cls = func.owner;

    // Point the code stream to the byte code, if any.
    if(func.isNormal)
      cur.code.setData(func.bcode, func.lines);

    assert(!func.isIdle, "don't use fstack.push() on idle functions");
  }

  // Set the stack point up as a state
  void push(State *st, MonsterObject *obj)
  {
    push(obj);
    cur.ftype = SPType.State;
    cur.state = st;

    assert(obj !is null);
    cur.cls = obj.cls;

    // Set up the byte code
    cur.code.setData(st.bcode, st.lines);
  }

  // Native constructor
  void pushNConst(MonsterObject *obj)
  {
    assert(obj !is null);
    push(obj);
    cur.ftype = SPType.NConst;
  }

  private void pushIdleCommon(Function *fn, MonsterObject *obj, SPType tp)
  {
    // Not really needed - we will allow static idle functions later
    // on.
    assert(obj !is null);

    push(obj);
    cur.func = fn;
    assert(fn.isIdle, fn.name.str ~ "() is not an idle function");
    cur.ftype = tp;
  }

  // These are used for the various idle callbacks
  void pushIdleInit(Function *fn, MonsterObject *obj)
  { pushIdleCommon(fn, obj, SPType.Idle_Initiate); }

  void pushIdleReentry(Function *fn, MonsterObject *obj)
  { pushIdleCommon(fn, obj, SPType.Idle_Reentry); }

  void pushIdleAbort(Function *fn, MonsterObject *obj)
  { pushIdleCommon(fn, obj, SPType.Idle_Abort); }

  void pushIdleCheck(Function *fn, MonsterObject *obj)
  { pushIdleCommon(fn, obj, SPType.Idle_Check); }

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
