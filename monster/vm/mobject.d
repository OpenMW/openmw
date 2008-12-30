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

import monster.compiler.states;
import monster.compiler.variables;
import monster.compiler.scopes;

import std.string;
import std.stdio;
import std.utf;

// An index to a monster object.
typedef int MIndex;

struct MonsterObject
{
  /*******************************************************
   *                                                     *
   *     Public variables                                *
   *                                                     *
   *******************************************************/

  MonsterClass cls;

  // Extra data. This allows you to assign additional data to an
  // object. We might refine this concept a little later.
  void *extra;

  // The thread. Each object has its own thread, but not every
  // MonsterObject has its own unique thread. For derived classes, we
  // allocate a MonsterObject for each parent class, but only one
  // thread for the object entire object.
  CodeThread *thread;

  // Object data segment
  int[] data;

  // Static data segment. Do not write to this.
  int[] sdata;

  // Parent object tree. This reflects the equivalent 'tree' table in
  // the MonsterClass.
  MonsterObject* tree[];


  /*******************************************************
   *                                                     *
   *     Functions for object handling                   *
   *                                                     *
   *******************************************************/

  // Get the next object in the objects list - used to iterate through
  // objects of one class
  MonsterObject *getNext()
  {
    // TODO: This syntax is rather hackish, and bug-prone if we
    // suddenly change the list structure.
    return cast(MonsterObject*)
      ( cast(MonsterClass.ObjectList.TList.Iterator)this ).getNext();
  }

  // Get the index of this object
  MIndex getIndex()
  {
    return cast(MIndex)( MonsterClass.ObjectList.getIndex(this)+1 );
  }

  // Delete this object. Do not use the object after calling this
  // function.
  void deleteSelf()
  {
    cls.deleteObject(this);
  }

  // Create a clone of this object. Note that this will always clone
  // and return the top object (thread.topObj), regardless of which
  // object in the list it is called on. In other words, the class
  // mo.cls is not always the same as mo.clone().cls.
  MonsterObject *clone()
  {
    auto t = thread.topObj;
    return t.cls.createClone(t);
  }

  /*******************************************************
   *                                                     *
   *     Casting / polymorphism functions                *
   *                                                     *
   *******************************************************/

  // Cast this object to the given class, if possible. Both upcasts
  // and downcasts are allowed.
  MonsterObject *Cast(MonsterClass toClass)
  { return doCast(toClass, thread.topObj.tree); }

  // Upcast this object to the given class. Upcasting means that
  // toClass must be the class of this object, or one of its parent
  // classes.
  MonsterObject *upcast(MonsterClass toClass)
  {
    assert(toClass !is null);
    return doCast(toClass, tree);
  }
  // Special version used from bytecode. The index is the global class
  // index.
  MonsterObject *upcastIndex(int index)
  {
    // Convert the global class index to the tree index. TODO: Later
    // on we should pass this index directly, but that is just
    // optimization.
    index = global.getClass(cast(CIndex)index).treeIndex;

    assert(index < tree.length, "cannot upcast class " ~ cls.getName ~
           " to index " ~ format(index));
    return tree[index];
  }

  // Is this object part of a linked inheritance chain?
  bool isBaseObject() {return !isTopObject(); }

  // Is this object the topmost object in the inheritance chain?
  bool isTopObject() { return thread.topObj is this; }


  /*******************************************************
   *                                                     *
   *     Member variable getters / setters               *
   *                                                     *
   *******************************************************/

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
    MonsterObject *obj = upcast(mc);

    // Return the pointer
    return cast(T*) obj.getDataInt(vb.number);
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
  int *getDataInt(int pos)
  {
    if(pos < 0 || pos>=data.length)
      fail("MonsterObject: data pointer out of range: " ~ toString(pos));
    return &data[pos];
  }

  // Get a long (two ints) from the data segment
  long *getDataLong(int pos)
  {
    if(pos < 0 || pos+1>=data.length)
      fail("MonsterObject: data pointer out of range: " ~ toString(pos));
    return cast(long*)&data[pos];
  }

  // Get an array from the data segment
  int[] getDataArray(int pos, int len)
  {
    if(pos < 0 || len < 0 || (pos+len) > data.length)
      fail("MonsterObject: data array out of range: pos=" ~ toString(pos) ~
           ", len=" ~toString(len));
    return data[pos..pos+len];
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
    thread.topObj.cls.findFunction(name).call(this);
  }

  // Call a function non-virtually. In other words, ignore
  // derived objects.
  void nvcall(char[] name)
  {
    assert(0, "not implemented");
  }

  // Set the current state of the object. If called from within state
  // code, we have to return all the way back to the state code level
  // before the new state is scheduled. If called when the object is
  // idle (not actively running state code), the state is scheduled
  // now, and the idle function is aborted. New state code does not
  // start running until the next frame.
  void setState(State *st, StateLabel *lb = null)
  {
    assert(st !is null || lb is null,
           "If state is null, label must also be null");
    thread.setState(st, lb);
  }

  // Named version of the above function. An empty string sets the
  // state to -1 (the empty state.) If no label is given (or given as
  // ""), this is equivalent to the script command state=name; If a
  // label is given, it is equivalent to state = name.label;
  void setState(char[] name, char[] label = "")
  {
    if(label == "")
      {
        if(name == "") thread.setState(null,null);
        else setState(cls.findState(name));
        return;
      }

    assert(name != "", "The empty state cannot contain the label " ~ label);

    auto stl = cls.findState(name, label);
    setState(stl.state, stl.label);
  }

  /*******************************************************
   *                                                     *
   *     Private functions                               *
   *                                                     *
   *******************************************************/
  private:

  MonsterObject *doCast(MonsterClass toClass, MonsterObject* ptree[])
  {
    assert(toClass !is null);

    if(toClass is cls) return this;

    // TODO: At some point, a class will have several possible tree
    // indices. We will loop through the list and try them all.
    int index = toClass.treeIndex;
    MonsterObject *mo = null;

    if(index < ptree.length)
      {
        mo = ptree[index];

        assert(mo !is this);

        // It's only a match if the classes match
        if(mo.cls !is toClass) mo = null;
      }

    // If no match was found, then the cast failed.
    if(mo is null)
      fail("object of class " ~ cls.name.str ~
           " cannot be cast to " ~ toClass.name.str);

    return mo;
  }
}

// Convert an index to an object pointer
MonsterObject *getMObject(MIndex index)
{
  if(index == 0)
    fail("Null object reference encountered");

  if(index < 0 || index > getTotalObjects())
    fail("Invalid object reference");

  MonsterObject *obj = MonsterClass.ObjectList.getNode(index-1);

  if(obj.thread == null)
    fail("Dead object reference (index " ~ toString(cast(int)index) ~ ")");

  assert(obj.getIndex() == index);

  return obj;
}

// Get the total number of MonsterObjects ever allocated for the free
// list. Does NOT correspond to the number of objects in use.
int getTotalObjects()
{
  return MonsterClass.ObjectList.totLength();
}
