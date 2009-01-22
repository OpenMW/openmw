/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (mobject.d) is part of the Monster script language package.

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

module monster.vm.mobject;

import monster.vm.thread;
import monster.vm.error;
import monster.vm.mclass;
import monster.vm.arrays;
import monster.vm.stack;

import monster.util.freelist;
import monster.util.list;

import monster.compiler.states;
import monster.compiler.variables;
import monster.compiler.scopes;
import monster.compiler.functions;

import std.string;
import std.stdio;
import std.utf;

// An index to a monster object.
typedef int MIndex;

union SharedType
{
  int i;
  uint ui;
  long l;
  ulong ul;
  float f;
  double d;

  void *vptr;
  Object obj;
}

struct ExtraData
{
  SharedType extra;
  vpNode node;
}

struct MonsterObject
{
  /*******************************************************
   *                                                     *
   *     Public variables                                *
   *                                                     *
   *******************************************************/

  MonsterClass cls;

  // Thread used for running state code. May be null if no code is
  // running or scheduled.
  Thread *sthread;

  // The following variables are "tree-indexed". This means that
  // they're arrays, with one element for each class in the
  // inheritance hierarchy. The corresponding class tree can be found
  // in cls.tree.

  // Object data segment.
  int[][] data;

  /*******************************************************
   *                                                     *
   *     Private variables                               *
   *                                                     *
   *******************************************************/

  //private:
  State *state;  // Current state, null is the empty state.

  public:

  /*******************************************************
   *                                                     *
   *     Functions for object handling                   *
   *                                                     *
   *******************************************************/

  // Get the index of this object
  MIndex getIndex()
  {
    return cast(MIndex)( ObjectList.getIndex(this)+1 );
  }

  // Delete this object. Do not use the object after calling this
  // function.
  void deleteSelf()
  {
    cls.deleteObject(this);
  }

  // Create a clone of this object.
  MonsterObject *clone()
  { return cls.createClone(this);  }

  /*******************************************************
   *                                                     *
   *     Member variable getters / setters               *
   *                                                     *
   *******************************************************/

  // The last two ints of the data segment can be used to store extra
  // data associated with the object. A typical example is the pointer
  // to a D/C++ struct or class counterpart to the Monster class.
  static const exSize = ExtraData.sizeof / int.sizeof;
  static assert(exSize*4 == ExtraData.sizeof);
  SharedType *getExtra(int index)
  {
    return & (cast(ExtraData*)&data[index][$-exSize]).extra;
  }
  SharedType *getExtra(MonsterClass mc)
  { return getExtra(cls.upcast(mc)); }

  // This is the work horse for all the set/get functions.
  T* getPtr(T)(char[] name)
  {
    // Find the variable
    Variable *vb = cls.findVariable(name);
    assert(vb !is null);

    // Check the type
    if(!vb.type.isDType(typeid(T)))
      {
        char[] request;
        static if(is(T == dchar)) request = "char"; else
        static if(is(T == AIndex)) request = "array"; else
        static if(is(T == MIndex)) request = "object"; else
          request = typeid(T).toString();

        fail(format("Requested variable %s is not the right type (wanted %s, found %s)",
                    name, request, vb.type.toString()));
      }

    // Cast the object to the right kind
    assert(vb.sc.isClass(), "variable must be a class variable");
    MonsterClass mc = vb.sc.getClass();
    assert(mc !is null);

    // Return the pointer
    return cast(T*) getDataInt(mc.treeIndex, vb.number);
  }
  T getType(T)(char[] name)
  { return *getPtr!(T)(name); }
  void setType(T)(char[] name, T t)
  { *getPtr!(T)(name) = t; }

  alias getPtr!(int) getIntPtr;
  alias getPtr!(uint) getUintPtr;
  alias getPtr!(long) getLongPtr;
  alias getPtr!(ulong) getUlongPtr;
  alias getPtr!(bool) getBoolPtr;
  alias getPtr!(float) getFloatPtr;
  alias getPtr!(double) getDoublePtr;
  alias getPtr!(dchar) getCharPtr;
  alias getPtr!(AIndex) getAIndexPtr;
  alias getPtr!(MIndex) getMIndexPtr;

  alias getType!(int) getInt;
  alias getType!(uint) getUint;
  alias getType!(long) getLong;
  alias getType!(ulong) getUlong;
  alias getType!(bool) getBool;
  alias getType!(float) getFloat;
  alias getType!(double) getDouble;
  alias getType!(dchar) getChar;
  alias getType!(AIndex) getAIndex;
  alias getType!(MIndex) getMIndex;

  alias setType!(int) setInt;
  alias setType!(uint) setUint;
  alias setType!(long) setLong;
  alias setType!(ulong) setUlong;
  alias setType!(bool) setBool;
  alias setType!(float) setFloat;
  alias setType!(double) setDouble;
  alias setType!(dchar) setChar;
  alias setType!(AIndex) setAIndex;
  alias setType!(MIndex) setMIndex;

  MonsterObject *getObject(char[] name)
  { return getMObject(getMIndex(name)); }
  void setObject(char[] name, MonsterObject *obj)
  { setMIndex(name, obj.getIndex()); }

  // Array stuff
  ArrayRef* getArray(char[] name)
  { return arrays.getRef(getAIndex(name)); }
  void setArray(char[] name, ArrayRef *r)
  { setAIndex(name,r.getIndex()); }

  char[] getString8(char[] name)
  { return toUTF8(getArray(name).carr); }
  void setString8(char[] name, char[] str)
  { setArray(name, arrays.create(toUTF32(str))); }


  /*******************************************************
   *                                                     *
   *     Lower level member data functions               *
   *                                                     *
   *******************************************************/

  // Get an int from the data segment
  int *getDataInt(int treeIndex, int pos)
  {
    assert(treeIndex >= 0 && treeIndex < data.length,
           "tree index out of range: " ~ toString(treeIndex));
    assert(pos >= 0 && pos<data[treeIndex].length,
           "data pointer out of range: " ~ toString(pos));
    return &data[treeIndex][pos];
  }

  // Get an array from the data segment
  int[] getDataArray(int treeIndex, int pos, int len)
  {
    assert(len > 0);
    assert(treeIndex >= 0 && treeIndex < data.length,
           "tree index out of range: " ~ toString(treeIndex));
    assert(pos >= 0 && (pos+len)<=data[treeIndex].length,
           "data pointer out of range: pos=" ~ toString(pos) ~
           ", len=" ~toString(len));
    return data[treeIndex][pos..pos+len];
  }


  /*******************************************************
   *                                                     *
   *     Calling functions and setting states            *
   *                                                     *
   *******************************************************/

  // Call a named function. The function is executed immediately, and
  // call() returns when the function is finished. The function is
  // called virtually, so any child class function that overrides it
  // will take precedence.
  void call(char[] name)
  {
    cls.findFunction(name).call(this);
  }

  // Create a paused thread that's set up to call the given
  // function. It must be started with Thread.call() or
  // Thread.restart().
  Thread *thread(char[] name)
  { return thread(cls.findFunction(name)); }
  Thread *thread(Function *fn)
  {
    assert(fn !is null);
    if(fn.paramSize > 0)
      fail("thread(): function " ~ fn.name.str ~ " cannot have parameters");

    Thread *trd = Thread.getNew();

    // Schedule the function to run the next frame
    trd.pushFunc(fn, this);
    assert(trd.isPaused);
    assert(trd.fstack.cur !is null);

    // pushFunc will mess with the stack frame though, so fix it.
    trd.fstack.cur.frame = stack.getStart();
    if(cthread !is null)
      cthread.fstack.restoreFrame();

    return trd;
  }

  // Create a thread containing the function and schedule it to start
  // the next frame
  Thread *start(char[] name)
  { return start(cls.findFunction(name)); }
  Thread *start(Function *fn)
  {
    assert(fn !is null);
    auto trd = thread(fn);
    trd.restart();
    return trd;
  }

  // Call a function non-virtually. In other words, ignore
  // derived objects.
  void nvcall(char[] name)
  {
    assert(0, "not implemented");
  }

  /* Set state. Invoked by the statement "state = statename;". This
     function can be called in several situations, with various
     results:

     + setState called with current state, no label
       -> no action is performed

     + setState called with another state
     + setState called with current state + a label
       -> state is changed normally

     If a state change takes place directly in state code, the code is
     aborted immediately. If it takes place in a function called from
     state code, then code flow is allowed to return normally back to
     the state code level, but is aborted immediately once it reaches
     state code.

     State changes outside state code will always unschedule any
     previously scheduled code (such as idle functions, or previous
     calls to setState.)
   */
  void setState(State *st, StateLabel *label)
  {
    // Does the state actually change?
    if(st !is state)
      {
        // Set the state
        state = st;

        // We must handle state functions and other magic here.
      }
    // If no label is specified and we are already in this state, then
    // don't do anything.
    else if(label is null) return;

    // Do we already have a thread?
    if(sthread !is null)
      {
        // Check if the thread has gone and died on us while we were
        // away.
        if(sthread.isDead)
          sthread = null;
        else
          // Still alive. Stop any execution of the thread
          sthread.stop();
      }

    // If we are jumping to anything but the empty state, we will have
    // to schedule some code.
    if(st !is null)
      {
        // Check that this state is valid
        assert(st.owner.parentOf(cls), "state '" ~ st.name.str ~
               "' is not part of class " ~ cls.getName());

        if(label is null)
          // Use the 'begin:' label, if any. It will be null there's
          // no begin label.
          label = st.begin;

        if(label !is null)
          {
            // Make sure there's a thread to run in
            if(sthread is null)
              sthread = Thread.getNew();

            // Schedule the thread to start at the given state and
            // label
            sthread.scheduleState(this, label.offs);
            assert(sthread.isScheduled);
          }
      }

    // If nothing is scheduled, kill the thread
    if(sthread !is null && !sthread.isScheduled)
      {
        assert(sthread.isTransient);
        sthread.kill();

        // Zero out any pointers to the thread.
        if(sthread is cthread)
          cthread = null;
        sthread = null;
      }

    assert(sthread is null || sthread.isScheduled);
  }

  void clearState() { setState(cast(State*)null, null); }

  // Index version of setState - called from bytecode
  void setState(int st, int label, int clsInd)
  {
    if(st == -1)
      {
        assert(label == -1);
        clearState();
        return;
      }

    auto cls = cls.upcast(clsInd);

    // TODO: This does not support virtual states yet
    auto pair = cls.findState(st, label);

    assert(pair.state.index == st);
    assert(pair.state.owner is cls);

    setState(pair.state, pair.label);
  }

  // Named version of the above function. An empty string sets the
  // state to -1 (the empty state.) If no label is given (or given as
  // ""), this is equivalent to the script command state=name; If a
  // label is given, it is equivalent to state = name.label;
  void setState(char[] name, char[] label = "")
  {
    if(label == "")
      {
        if(name == "") clearState();
        else setState(cls.findState(name), null);
        return;
      }

    assert(name != "", "The empty state cannot contain the label " ~ label);

    auto stl = cls.findState(name, label);
    setState(stl.state, stl.label);
  }
}

alias FreeList!(MonsterObject) ObjectList;

// The freelist used for allocation of objects. This contains all
// allocated and in-use objects.
ObjectList allObjects;

// Convert an index to an object pointer
MonsterObject *getMObject(MIndex index)
{
  if(index == 0)
    fail("Null object reference encountered");

  if(index < 0 || index > ObjectList.totLength())
    fail("Invalid object reference");

  MonsterObject *obj = ObjectList.getNode(index-1);

  if(obj.cls is null)
    fail("Dead object reference (index " ~ toString(cast(int)index) ~ ")");

  assert(obj.getIndex() == index);

  return obj;
}
