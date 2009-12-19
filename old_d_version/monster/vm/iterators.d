/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (iterators.d) is part of the Monster script language
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

module monster.vm.iterators;

import monster.util.freelist;
import monster.vm.error;
import monster.vm.arrays;
import monster.vm.mclass;
import monster.vm.mobject;
import monster.util.flags;

import std.string;
import std.stdio;

// An iterator index.
typedef int IIndex;

// Flags for iterator structs
enum IFlags
  {
    None        = 0x00,
    Alive       = 0x01, // This reference is not deleted
  }

struct IteratorRef
{
  Flags!(IFlags) flags;

  ArrayRef *array;
  int index; // TODO: Might not be necessary to keep a local copy of this
  int indexMul; // Index multiplied with element size
  int elemSize;
  int *sindex; // Index on the stack
  int[] sval; // Value on the stack

  bool isReverse, isRef;
  bool isClass;

  MonsterObject *mo;
  MonsterClass mc;

  // Array iterators
  bool firstArray(bool irev, bool iref, int *stk)
  {
    isRef = iref;
    isReverse = irev;
    isClass = false;

    // Replace the array index on the stack
    AIndex ai = cast(AIndex)*stk;
    *stk = cast(int) getIndex();

    // Fetch the array
    array = arrays.getRef(ai);

    // Cannot use reference values on const arrays
    if(array.isConst && isRef)
      // TODO: Try to give line and file in all messages
      fail("Cannot use 'ref' values with constant arrays");

    // Skip the loop if it's empty
    if(array.iarr.length == 0) return false;

    assert(array.elemSize > 0);
    elemSize = array.elemSize;

    // Point to the stack index and value
    stk -= elemSize;
    sval = stk[0..elemSize];
    stk--;
    sindex = stk;

    // Set up the first element
    if(isReverse) index = array.length-1;
    else index = 0;

    indexMul = index * elemSize;

    *sindex = index;
    sval[] = array.iarr[indexMul..indexMul+elemSize];

    return true;
  }

  // Class iterators
  bool firstClass(MonsterClass mc, int[] stk)
  {
    assert(stk.length == 2);
    isClass = true;

    // Set the iterator index on the stack
    stk[1] = cast(int) getIndex();

    mo = mc.getFirst();
    this.mc = mc;

    // Are there any objects?
    if(mo == null) return false;

    sindex = &stk[0];
    *sindex = cast(int)mo.getIndex();
    return true;
  }
  
  void storeRef()
  {
    assert(!isClass);

    if(isRef)
      array.iarr[indexMul..indexMul+elemSize] = sval[];
    else
      fail("Array iterator update called on non-ref parameter");
  }

  bool next()
  {
    // Handle class iterations seperately
    if(isClass)
      {
        mo = mc.getNext(mo);
        if(mo == null) return false;

        *sindex = cast(int)mo.getIndex();

        return true;
      }

    if(isReverse)
      {
        index--;
        indexMul -= elemSize;
        if(index == -1) return false;
      }
    else
      {
        index++;
        indexMul += elemSize;
        if(index*elemSize == array.iarr.length) return false;
      }

    assert(indexMul < array.iarr.length);
    assert(index >= 0 && index < array.length);
    assert(indexMul == index*elemSize);

    *sindex = index;
    sval[] = array.iarr[indexMul..indexMul+elemSize];

    return true;
  }

  IIndex getIndex()
  {
    return cast(IIndex)( Iterators.IterList.getIndex(this) );
  }

  bool isAlive() { return flags.has(IFlags.Alive); }
}

Iterators iterators;

struct Iterators
{
  alias FreeList!(IteratorRef) IterList;

  private:
  IterList iterList;

  // Get a new iterator reference
  IteratorRef *createIterator()
  {
    IteratorRef *it = iterList.getNew();

    assert(!it.isAlive);

    // Set the "alive" flag
    it.flags.set(IFlags.Alive);

    return it;
  }

  // Put a reference back into the freelist
  void destroyIterator(IteratorRef *it)
  {
    assert(it.isAlive);
    it.flags.unset(IFlags.Alive);
    iterList.remove(it);
  }

  public:

  bool firstArray(bool irev, bool iref, int *stk)
  {
    IteratorRef *it = createIterator();
    bool res = it.firstArray(irev,iref,stk);

    // Kill the iterator reference if we are done iterating
    if(!res) destroyIterator(it);
    return res;
  }

  bool firstClass(MonsterClass mc, int[] stk)
  {
    IteratorRef *it = createIterator();
    bool res = it.firstClass(mc,stk);

    // Kill the iterator reference if we are done iterating
    if(!res) destroyIterator(it);
    return res;
  }

  bool next(IIndex ind)
  {
    IteratorRef *it = getRef(ind);
    bool res = it.next();

    // Kill the iterator reference if this was the last iteration
    if(!res) destroyIterator(it);
    return res;
  }

  void stop(IIndex ind)
  {
    IteratorRef *it = getRef(ind);
    destroyIterator(it);
  }

  void update(IIndex ind)
  {
    IteratorRef *it = getRef(ind);
    it.storeRef();
  }

  IteratorRef *getRef(IIndex index)
  {
    if(index < 0 || index >= getTotalIterators())
      fail("Invalid iterator reference: " ~ toString(cast(int)index));

    IteratorRef *itr = IterList.getNode(index);

    if(!itr.isAlive)
      fail("Dead iterator reference: " ~ toString(cast(int)index));

    assert(itr.getIndex() == index);

    return itr;
  }

  // Get the number of iterator references in use
  int getIterators()
  {
    return iterList.length();
  }

  // Get the total number of Iterator references ever allocated for the
  // free list.
  int getTotalIterators()
  {
    return IterList.totLength();
  }
}
