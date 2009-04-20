/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (growarray.d) is part of the Monster script language
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

module monster.util.growarray;

// Array that grows without reallocations.
struct GrowArray(T)
{
  const defSize = 128;

 private:
  uint listSize = defSize; // Size of new lists
  uint elements; 	   // Current number of elements
  uint elemsAlloc;	   // Elements allocated

  uint begin;		   // At what element to start counting. Used for
			   // slices.

  T[][] listList;

  // Make sure there is room for at least 'size' elements in total.
  void alloc(uint size)
    {
      // Do nothing if the list is large enough
      if(size <= elemsAlloc) return;

      // If this is a slice, we must always reallocate when
      // growing. Implement that later.
      if(begin) assert(0, "Cannot grow a slice");

      // Number of lists we need
      uint lists = ((size-1) / listSize) + 1;

      // The number of needed elements should never decrease
      assert((listSize*lists) >= elemsAlloc);

      // Number of elements we need allocated
      elemsAlloc = listSize * lists;

      // Make sure the list of lists is large enough
      if(listList.length < lists)
	listList.length = lists+30;

      // Allocate the lists we need
      for(int i=0; i<lists; i++)
	listList[i].length = listSize;
    }

 public:
  uint length() { return elements - begin; }
  void length(uint newLen)
    {
      newLen += begin;

      alloc(newLen);

      elements = newLen;
    }

  // Initialize - set the current size
  void initialize(uint size = 0, uint listSize = defSize)
    {
      assert(listList.length == 0);
      this.listSize = listSize;
      length(size);
    }

  static GrowArray opCall(uint size = 0, uint listSize = defSize)
    {
      GrowArray a;
      a.initialize(size, listSize);
      return a;
    }

  void opCatAssign(T t)
    {
      length = length + 1;
      opIndexAssign(t, length-1);
    }

  void opCatAssign(T[] list)
    {
      uint len = length;
      length = len + list.length;
      foreach(int i, ref T t; list)
	opIndexAssign(t, len+i);
    }

  T opIndex(int index)
    {
      index += begin;
      assert(index >= begin && index < elements,
	     "GrowArray index out of bounds");

      return listList[index/listSize][index%listSize];
    }

  T opIndexAssign(T value, int index)
    {
      index += begin;
      assert(index >= begin && index < elements,
	     "GrowArray index out of bounds");

      return (listList[index/listSize][index%listSize] = value);
    }

  T* getPtr(int index)
    {
      index += begin;
      assert(index >= begin && index < elements,
	     "GrowArray index out of bounds");

      return &listList[index/listSize][index%listSize];
    }

  GrowArray opSlice(int start, int stop)
    {
      assert(start<=stop, "Illegal GrowArray slice");
      GrowArray ga = *this;
      ga.begin = begin+start;
      ga.length = stop-start;
      return ga;
    }

  GrowArray opSlice()
    {
      return *this;
    }

  // Get a contiguous array copy containg all the elements.
  T[] arrayCopy()
    {
      T[] res = new T[length()];

      // Non-optimized!
      foreach(i, ref r; res)
        r = opIndex(i);

      return res;
    }

  int opApply(int delegate(ref int, ref T) dg)
    {
      int res;
      int len = length;
      int pos = begin%listSize;
      int list = begin/listSize;
      for(int i; i<len; i++)
	{
	  res = dg(i, listList[list][pos++]);
	  if(res) break;

	  if(pos == listSize)
	    {
	      list++;
	      pos = 0;
	    }
	}
      return res;
    }

  int opApply(int delegate(ref T) dg)
    {
      int res;
      int len = length;
      int pos = begin%listSize;
      int list = begin/listSize;
      for(int i; i<len; i++)
	{
	  res = dg(listList[list][pos++]);
	  if(res) break;

	  if(pos == listSize)
	    {
	      list++;
	      pos = 0;
	    }
	}
      return res;
    }
}

unittest
{
  // Test default initialization
  GrowArray!(int) arr;
  assert(arr.begin == 0);
  assert(arr.length == 0);
  assert(arr.elements == 0);
  assert(arr.elemsAlloc == 0);
  assert(arr.listList.length == 0);
  assert(arr.listSize == 128);

  // Setting length
  arr.length = 3;
  assert(arr.length == 3);
  assert(arr.elements == 3);
  assert(arr.elemsAlloc == arr.listSize);
  assert(arr.listList.length >= 1);

  // Setting and reading elements
  arr[0] = 1;
  arr[1] = 2;
  arr[2] = 3;
  assert(arr[0] == 1);
  assert(arr[1] == 2);
  assert(arr[2] == 3);
  assert(arr.listList[0][0] == 1);
  assert(arr.listList[0][1] == 2);
  assert(arr.listList[0][2] == 3);

  // Test opCatAssign
  arr ~= 4;
  assert(arr.length == 4);
  assert(arr[3] == 4);

  // Foreach
  int tmp = 0;
  foreach(int i, int v; arr)
    {
      assert(v==i+1);
      tmp++;
    }
  assert(tmp == 4);

  tmp = 1;
  foreach(int v; arr)
    assert(v == tmp++);
  assert(tmp == 5);

  // Slicing the entire array
  arr = arr[0..4];
  assert(arr.length == 4);
  assert(arr[3] == 4);

  // Slicing part of the array
  auto arrS = arr[1..3];
  assert(arrS.length == 2);
  assert(arrS[0] == 2);
  assert(arrS[1] == 3);
  arrS[0] = 10;
  assert(arr[1] == 10);

  // Slicing the slice
  arrS = arrS[1..2];
  assert(arrS.length == 1);
  assert(arrS[0] == 3);

  // Empty slice
  arrS = arr[3..3];
  assert(arrS.length == 0);

  // Custom list size, and more than one list
  auto arr2 = GrowArray!(byte)(3,2);
  assert(arr2.length == 3);
  assert(arr2.elements == 3);
  assert(arr2.listSize == 2);
  assert(arr2.elemsAlloc == 4);
  assert(arr2.listList.length >= 2);
  assert(arr2.listList[0].length == 2);

  assert(arr2[0] == 0);
  assert(arr2[1] == 0);
  assert(arr2[2] == 0);

  arr2[1]=2;
  arr2[2]=4;

  foreach(int i, byte v; arr2)
    assert(v == 2*i);

  // Check that boundry checking works (in non-release mode.)
  bool err = false;
  try{arr2[3];}
  catch
    {
      err = true;
    }
  assert(err == true);

  err = false;
  try{arr2[3] = 0;}
  catch
    {
      err = true;
    }
  assert(err == true);
}
