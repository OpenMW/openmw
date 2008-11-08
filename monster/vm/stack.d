/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (stack.d) is part of the Monster script language
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

module monster.vm.stack;

import std.string;
import std.stdio;
import std.utf;

import monster.compiler.scopes;

import monster.vm.mobject;
import monster.vm.mclass;
import monster.vm.arrays;
import monster.vm.error;

// The stack. One nice attribue of our cooperative multithreading
// scheme is that we only need one single stack frame. All functions
// and code is required to "finish up" before returning or giving
// control to other code. When we switch to "real" threads later, we
// will need one stack per system thread, but many virtual threads
// will still share each stack.
CodeStack stack;

// A simple stack frame. All data are in chunks of 4 bytes
struct CodeStack
{
  private:
  int[] data;

  int left, total;
  int *pos; // Current position

  // Frame pointer, used for accessing local variables and parameters
  int *frame;
  int fleft; // A security measure to make sure we don't access
	     // variables outside the stack

  public:
  void init()
  {
    // 100 is just a random number - it should probably be quite a bit
    // larger (but NOT dynamic, since we want to catch run-away code.)
    const int size = 100;

    data.length = size;
    left = size;
    total = size;
    pos = data.ptr;
    frame = null;
  }

  // Get the current position index. Used mostly for debugging and
  // error checking.
  int getPos()
  {
    return total-left;
  }

  // Sets the current position as the 'frame pointer', and return
  // previous value.
  int *setFrame()
  {
    auto old = frame;
    frame = pos;
    fleft = left;
    //writefln("setFrame(): new=%s, old=%s", frame, old);
    return old;
  }

  // Sets the given frame pointer
  void setFrame(int *frm)
  {
    //writefln("setFrame(%s)", frm);
    frame = frm;
    if(frm is null)
      {
        fleft = 0;
        return;
      }
    fleft = left + (pos-frm);
    assert(fleft >= 0 && fleft <= total);
  }

  // Reset the stack level to zero. Should only be called when the
  // frame pointer is already zero (we use it in state code only.)
  void reset()
  {
    left = total;
    pos = data.ptr;

    assert(fleft == left);
  }

  void pushInt(int i)
  {
    left--;
    if(left<0) overflow("pushInt");
    *pos = i;
    pos++;
  }

  void pushLong(long i)
  {
    left -= 2;
    if(left<0) overflow("pushLong");
    *(cast(long*)pos) = i;
    pos+=2;
  }

  int popInt()
  {
    left++;
    if(left>total) overflow("popInt");
    pos--;
    return *pos;
  }

  long popLong()
  {
    left+=2;
    if(left>total) overflow("popLong");
    pos-=2;
    return *(cast(long*)pos);
  }

  // Get the pointer to an int at the given position backwards from
  // the current stack pointer. 0 means the first int, ie. the one we
  // would get if we called popInt. 1 is the next, etc
  int *getInt(int ptr)
  {
    ptr++;
    if(ptr < 1 || ptr > (total-left) )
      fail("CodeStack.getInt() pointer out of range");
    return pos-ptr;
  }

  // Get the array _beginning_ at ptr
  int[] getInts(int ptr, int len)
  {
    assert(len > 0 && ptr >= len-1);
    if(left+len-ptr>total) overflow("getInts");
    return getInt(ptr)[0..len];
  }

  // Pops the next len ints off the stack and returns them as an
  // array. The array is ordered as the values were pushed, not as
  // they would have been popped (ie. this function is like popping
  // one big value of the stack.) The array is a direct slice of the
  // stack, so don't store it or use it after pushing other values.
  int[] popInts(int len)
  {
    assert(len > 0);
    int[] r = getInts(len-1, len);
    pop(len);
    return r;
  }

  void pushInts(int[] arr)
  {
    left -= arr.length;
    if(left<0) overflow("pushInts");
    pos[0..arr.length] = arr[];
    pos+=arr.length;
  }

  // Get an int a given position from frame pointer. Can be negative
  // or positive. 0 means the int at the pointer, -1 is the one before
  // and 1 the one after, etc.
  int *getFrameInt(int ptr)
  {
    assert(frame !is null);
    assert(frame <= pos);
    if(ptr < (fleft-total) || ptr >= fleft)
      fail("CodeStack.getFrameInt() pointer out of range");
    return frame+ptr;
  }

  // Pushing and poping objects of the stack - will actually push/pop
  // their index.
  void pushObject(MonsterObject *mo)
  { pushInt(mo.getIndex); }

  MonsterObject *popObject()
  { return getMObject(cast(MIndex)popInt()); }

  // Push an object, and make sure it is cast to the right type
  void pushCast(MonsterObject *obj, MonsterClass cls)
  { pushObject(obj.Cast(cls)); }

  void pushCast(MonsterObject *obj, char[] name)
  { pushCast(obj, global.getClass(name)); }

  // Push arrays of objects. TODO: These do memory allocation, and I'm
  // not sure that belongs here. I will look into it later.
  void pushObjects(MonsterObject *objs[])
  {
    int[] indices;
    indices.length = objs.length;

    foreach(i, mo; objs)
      indices[i] = mo.getIndex();

    pushIArray(indices);
  }

  MonsterObject*[] popObjects()
  {
    MIndex[] indices = cast(MIndex[]) popIArray();
    MonsterObject* objs[];

    objs.length = indices.length;
    foreach(i, ind; indices)
      objs[i] = getMObject(ind);

    return objs;
  }

  // Push and pop array references.
  void pushArray(ArrayRef *ar)
  { pushInt(ar.getIndex); }
  ArrayRef *popArray()
  { return arrays.getRef(cast(AIndex)popInt()); }
  ArrayRef *getArray(int i)
  { return arrays.getRef(cast(AIndex)*getInt(i)); }

  // More easy versions. Note that pushArray() will create a new array
  // reference each time it is called! Only use it if this is what you
  // want.
  void pushCArray(dchar[] str) { pushArray(arrays.create(str)); }
  void pushIArray(int[] str) { pushArray(arrays.create(str)); }
  void pushUArray(uint[] str) { pushArray(arrays.create(str)); }
  void pushLArray(long[] str){ pushArray(arrays.create(str)); }
  void pushULArray(ulong[] str) { pushArray(arrays.create(str)); }
  void pushFArray(float[] str) { pushArray(arrays.create(str)); }
  void pushDArray(double[] str) { pushArray(arrays.create(str)); }
  void pushAArray(AIndex[] str) { pushArray(arrays.create(str)); }

  alias pushCArray pushArray, pushString;
  alias pushIArray pushArray;
  alias pushFArray pushArray;
  alias pushAArray pushArray;
  alias pushString8 pushArray, pushString;

  dchar[] popCArray() { return popArray().carr; }
  int[] popIArray() { return popArray().iarr; }
  float[] popFArray() { return popArray().farr; }
  AIndex[] popAArray() { return popArray().aarr; }
  alias popCArray popString;

  void pushString8(char[] str)
  { pushArray(toUTF32(str)); }
  char[] popString8()
  { return toUTF8(popString()); }

  // For multibyte arrays
  void pushArray(int[] str, int size)
  { pushArray(arrays.create(str, size)); }


  // Various convenient conversion templates. These will be inlined,
  // so don't worry :) The *4() functions are for types that are 4
  // bytes long. These are mostly intended for use in native
  // functions, so there is no equivalent of getFrameInt and similar
  // functions.
  void push4(T)(T var)
  {
    static assert(T.sizeof == 4);
    pushInt(*(cast(int*)&var));
  }
  T pop4(T)()
  {
    static assert(T.sizeof == 4);
    int i = popInt();
    return *(cast(T*)&i);
  }
  T* get4(T)(int ptr) { return cast(T*)getInt(ptr); }

  // 64 bit version
  void push8(T)(T var)
  {
    static assert(T.sizeof == 8);
    pushLong(*(cast(long*)&var));
  }
  T pop8(T)()
  {
    static assert(T.sizeof == 8);
    long l = popLong();
    return *(cast(T*)&l);
  }

  // Bools are 1 byte in D
  void pushBool(bool b)
  {
    if(b) pushInt(1);
    else pushInt(0);
  }
  bool popBool() { return popInt() != 0; }
  alias get4!(bool) getBool;

  // Template conversions

  alias push4!(MIndex) pushIndex;
  alias pop4!(MIndex) popIndex;
  alias get4!(MIndex) getIndex;

  alias push4!(AIndex) pushAIndex;
  alias pop4!(AIndex) popAIndex;
  alias get4!(AIndex) getAIndex;

  alias push4!(uint) pushUint;
  alias pop4!(uint) popUint;
  alias get4!(uint) getUint;

  alias get4!(long) getLong;

  alias push8!(ulong) pushUlong;
  alias pop8!(ulong) popUlong;
  alias get4!(ulong) getUlong;

  alias push4!(float) pushFloat;
  alias pop4!(float) popFloat;
  alias get4!(float) getFloat;

  alias push8!(double) pushDouble;
  alias pop8!(double) popDouble;
  alias get4!(double) getDouble;

  alias push4!(dchar) pushChar;
  alias pop4!(dchar) popChar;
  alias get4!(dchar) getChar;

  // Pop off and ignore a given amount of values
  void pop(int num)
  {
    left += num;
    if(left>total) overflow("pop1");
    pos -= num;
  }

  // Pop off and ignore given values, but remember the top values
  void pop(uint num, uint keep)
  {
    assert(keep>0);
    assert(num>0);

    left += num;

    // We move the stack pointer back num values, but we access as far
    // back as num+keep values, so we need to check that we are still
    // within the stack.
    if((left+keep)>total) overflow("pop2");

    int *from = pos-keep; // Where to get the 'keep' values from
    int *to = from-num; // Where they end up
    pos -= num; // Where the final stack pointer should be

    assert(to < from);

    // Copy the values
    for(; keep>0; keep--)
      *(to++) = *(from++);
  }

  void debugPrint()
  {
    writefln("Stack:");
    foreach(int i, int val; data[0..total-left])
      writefln("%s: %s", i, val);
    writefln();
  }

  void overflow(char[] func)
  {
    char[] res;
    if(left<0)
      res = format("Stack overflow by %s ints in CodeStack.%s()",
		   -left, func);
    else if(left>total)
      res = format("Stack underflow by %s ints in CodeStack.%s()",
		   left-total, func);
    else res = format("Internal error in CodeStack.%s(), left=%s, total=%s",
		      func, left, total);
    fail(res);
  }
}
