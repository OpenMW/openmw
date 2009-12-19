/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
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
import monster.compiler.functions;
import monster.options;

import monster.vm.mobject;
import monster.vm.mclass;
import monster.vm.arrays;
import monster.vm.error;

// Stack. There's only one global instance, but threads will make
// copies when they need it.
CodeStack stack;

struct FunctionRef
{
  MIndex obj;
  int fIndex;

  MonsterObject *getObject()
  { return getMObject(obj); }

  Function *getFunctionNonVirtual()
  { return functionList[fIndex]; }

  Function *getFunction()
  {
    auto f = getFunctionNonVirtual();
    return f.findVirtual(getObject());
  }

  void set(Function* fn, MonsterObject *mo)
  {
    assert(fn !is null);
    assert(mo !is null);
    assert(mo.cls.childOf(fn.owner));
    fIndex = fn.getGIndex();
    obj = mo.getIndex();
  }
}
static assert(FunctionRef.sizeof == 8);

// A simple stack. All data are in chunks of 4 bytes
struct CodeStack
{
  private:
  int[] data;

  int left, total;
  int *pos; // Current position

  public:
  void init()
  {
    data.length = maxStack;
    left = maxStack;
    total = maxStack;
    pos = data.ptr;
  }

  // Get the current position index.
  int getPos()
  {
    return total-left;
  }

  // Reset the stack level to zero.
  void reset()
  {
    left = total;
    pos = data.ptr;
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
    assert(r.length == len);
    return r;
  }

  void pushInts(int[] arr)
  {
    left -= arr.length;
    if(left<0) overflow("pushInts");
    pos[0..arr.length] = arr[];
    pos+=arr.length;
  }

  // Pushing and poping objects of the stack - will actually push/pop
  // their index.
  void pushObject(MonsterObject *mo)
  { pushInt(mo.getIndex); }

  MonsterObject *popObject()
  { return getMObject(cast(MIndex)popInt()); }

  MonsterObject *peekObject()
  { return getMObject(cast(MIndex)peekInt()); }

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
  ArrayRef *peekArray()
  { return getArray(0); }

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
  void pushMArray(AIndex[] str) { pushArray(arrays.create(str)); }

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
  char[] peekString8()
  { return toUTF8(peekArray().carr); }

  // For multibyte arrays
  void pushArray(int[] str, int size)
  { pushArray(arrays.create(str, size)); }

  // Various convenient conversion templates. These will be inlined,
  // so don't worry :) The *4() functions are for types that are 4
  // bytes long.
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
  // Gets a pointer to a given stack value. Counts from the head - 0
  // is the first int, 1 is the second, etc. Note that it counts in
  // ints (four bytes) no matter what the type T is - this is by
  // design.
  T* get4(T)(int ptr) { return cast(T*)getInt(ptr); }

  // Returns the first value on the stack without poping it
  T peek4(T)() { return *(cast(T*)getInt(0)); }

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

  alias push4!(MIndex) pushMIndex;
  alias pop4!(MIndex) popMIndex;
  alias get4!(MIndex) getMIndex;
  alias peek4!(MIndex) peekMIndex;

  alias push4!(AIndex) pushAIndex;
  alias pop4!(AIndex) popAIndex;
  alias get4!(AIndex) getAIndex;
  alias peek4!(AIndex) peekAIndex;

  alias peek4!(int) peekInt;

  alias push4!(uint) pushUint;
  alias pop4!(uint) popUint;
  alias get4!(uint) getUint;
  alias peek4!(uint) peekUint;

  alias get4!(long) getLong;
  alias peek4!(long) peekLong;

  alias push8!(ulong) pushUlong;
  alias pop8!(ulong) popUlong;
  alias get4!(ulong) getUlong;
  alias peek4!(ulong) peekUlong;

  alias push4!(float) pushFloat;
  alias pop4!(float) popFloat;
  alias get4!(float) getFloat;
  alias peek4!(float) peekFloat;

  alias push8!(double) pushDouble;
  alias pop8!(double) popDouble;
  alias get4!(double) getDouble;
  alias peek4!(double) peekDouble;

  alias push4!(dchar) pushChar;
  alias pop4!(dchar) popChar;
  alias get4!(dchar) getChar;
  alias peek4!(dchar) peekDchar;

  alias push8!(FunctionRef) pushFuncRef;
  alias pop8!(FunctionRef) popFuncRef;
  alias get4!(FunctionRef) getFuncRef;
  alias peek4!(FunctionRef) peekFuncRef;

  void pushFuncRef(Function *fn, MonsterObject *obj)
  {
    FunctionRef f;
    f.set(fn,obj);
    pushFuncRef(f);
  }

  void pushFail(T)(T t)
  {
    static assert(0, "pushType not yet implemented for " ~ T.stringof);
  }

  T popFail(T)()
  {
    static assert(0, "popType not yet implemented for " ~ T.stringof);
  }

  // Generic push template
  template pushType(T)
  {
    static if(is(T == MIndex) || is(T == AIndex) ||
              is(T == int) || is(T == uint) || is(T == float))
      alias push4!(T) pushType;

    else static if(is(T == long) || is(T == ulong) ||
                   is(T == double) || is(T == dchar))
      alias push8!(T) pushType;

    else
      alias pushFail!(T) pushType;
  }

  // Ditto for pop
  template popType(T)
  {
    static if(is(T == MIndex) || is(T == AIndex) ||
              is(T == int) || is(T == uint) || is(T == float))
      alias pop4!(T) popType;

    else static if(is(T == long) || is(T == ulong) ||
                   is(T == double) || is(T == dchar))
      alias pop8!(T) popType;

    else
      alias popFail!(T) popType;
  }

  // Pop off and ignore a given amount of values
  void pop(int num)
  {
    left += num;
    if(left>total) overflow("pop1");
    pos -= num;
  }

  // Pop off and ignore given values, but remember the top
  // values. Equivalent to popping of (and storing) 'keep' ints, then
  // poping away 'num' ints, and finally pushing the kept ints
  // back. The final stack imprint is -num.
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

  private:
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
