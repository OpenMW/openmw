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
import monster.vm.thread;
import monster.compiler.states;
import monster.compiler.functions;
import monster.compiler.linespec;

import monster.util.freelist;

import std.stdio;
import std.string;

// "friendly" parameter and stack handling.
enum SPType
  {
    Function,      // A function (script or native)
    Idle,          // Idle function
    State,         // State code
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

  int *frame; // Stack frame for this function

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

  bool isIdle()
  { return ftype == SPType.Idle; }

  bool isNative()
  { return isFunc && func.isNative; }

  bool isNormal()
  { return isState || (isFunc && func.isNormal); }

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

    if(isFunc)
      fl.line = findLine(func.lines, pos);
    else
      fl.line = findLine(state.lines, pos);

    return fl;
  }

  char[] toString()
  {
    assert(func !is null);

    char[] type, cls, name;
    cls = getCls().name.str;
    if(isState)
      {
        type = "state";
        name = state.name.str;
      }
    else
      {
        assert(isFunc);
        name = func.name.str;

        if(isIdle) type = "idle";
        else if(isNormal) type = "script";
        else if(isNative) type = "native";
        else assert(0);
      }

    // Function location and name
    return format("%s %s.%s", type, cls, name);
  }
}

alias FreeList!(StackPoint) StackList;
alias StackList.TNode *StackNode;

struct FunctionStack
{
  private:
  // Guard against infinite recursion
  static const maxStack = 100;

  // Number of native functions on the stack
  int natives;

  public:

  StackList list;

  // The current entry
  StackPoint *cur = null;

  // Consistancy checks
  invariant()
  {
    if(cur !is null)
      {
        assert(list.length > 0);
        if(cur.ftype == SPType.State)
          assert(list.length == 1);
      }
    else assert(list.length == 0);
  }

  // Set the global stack frame pointer to correspond to the current
  // entry in the fstack. Must be called when putting a thread in the
  // foreground.
  void restoreFrame()
  {
    if(cur !is null)
      stack.setFrame(cur.frame);
    else
      stack.setFrame(null);
  }

  void killAll()
  {
    assert(natives == 0);

    while(list.length)
      {
        assert(cur !is null);
        list.remove(cur);
        cur = list.getHead();
      }
    assert(cur is null);
  }

  bool hasNatives() { return natives != 0; }

  // Check if the function calling us is a normal function
  bool isNormal()
  {
    return cur !is null && cur.isNormal;
  }

  // Is the function stack empty?
  bool isEmpty() { return cur is null; }

  bool isIdle() { return cur !is null && cur.ftype == SPType.Idle; }

  // Are we currently running state code?
  bool isStateCode() { return list.length == 1 && cur.ftype == SPType.State; }

  // Sets up the next stack point and assigns the given object
  private void push(MonsterObject *obj)
  {
    if(list.length >= maxStack)
      fail("Function stack overflow - infinite recursion?");

    assert(cur is null || !cur.isIdle,
           "Cannot call other script functions from an idle function");

    // Puts a new node at the beginning of the list
    cur = list.getNew();
    cur.obj = obj;
    cur.frame = stack.setFrame();
  }

  // Set the stack point up as a function. Allows obj to be null.
  void push(Function *func, MonsterObject *obj)
  {
    //assert(cthread !is null && this is &cthread.fstack);

    push(obj);

    assert(func !is null);
    cur.ftype = SPType.Function;
    cur.func = func;

    /*
    writefln("Pushing ", func.name);
    writefln(toString, "\n");
    */

    assert(func.owner !is null);
    assert(obj is null || func.owner.parentOf(obj.cls));

    // Point the code stream to the byte code, if any.
    if(func.isNormal)
      cur.code.setData(func.bcode);
    else if(func.isNative)
      natives++;

    assert(!func.isIdle, "don't use fstack.push() on idle functions");
  }

  // Set the stack point up as a state
  void push(State *st, MonsterObject *obj)
  {
    assert(st !is null);
    assert(isEmpty,
           "state code can only run at the bottom of the function stack");

    //writefln("Pushing state ", st.name);

    push(obj);
    cur.ftype = SPType.State;
    cur.state = st;

    assert(obj !is null);
    assert(st.owner !is null);
    assert(st.owner.parentOf(obj.cls));

    // Set up the byte code
    cur.code.setData(st.bcode);
  }

  void pushIdle(Function *func, MonsterObject *obj)
  {
    push(obj);
    assert(func !is null);
    cur.func = func;
    cur.ftype = SPType.Idle;

    //writefln("Pushing idle ", func.name);

    assert(func.owner !is null);
    assert(obj is null || func.owner.parentOf(obj.cls));
    assert(func.isIdle, func.name.str ~ "() is not an idle function");
  }

  // Pops one entry of the stack. Checks that the stack level has been
  // returned to the correct position.
  void pop()
  {
    if(isEmpty)
      fail("Function stack underflow");

    assert(list.length >= 1);

    if(cur.isNative)
      natives--;
    assert(natives >= 0);

    /*
    if(cur.isFunc) writefln("popping function ", cur.func.name);
    else if(cur.isState) writefln("popping state ", cur.state.name);
    writefln(toString, "\n");
    */

    // Remove the topmost node from the list, and set cur.
    assert(cur == list.getHead());
    list.remove(cur);
    cur = list.getHead();

    restoreFrame();

    assert(list.length != 0 || cur is null);
  }

  // Get a stack trace (pretty basic at the moment)
  char[] toString()
  {
    char[] res;

    int i;
    foreach(ref c; list)
      {
        char[] msg;
        if(i == 0)
          msg = " (<---- current function)";
        else if(i == list.length-1)
          msg = " (<---- start of function stack)";
        res = c.toString ~ msg ~ '\n' ~ res;
        i++;
      }

    return "Trace:\n" ~ res;
  }
}
