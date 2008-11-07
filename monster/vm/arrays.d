/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (arrays.d) is part of the Monster script language package.

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

module monster.vm.arrays;

import monster.vm.stack;
import monster.util.freelist;
import monster.util.flags;
import monster.vm.error;

import std.string;
import std.uni;
import std.stdio;

// An index to an array. Array indices may be 0, unlike object indices
// which span from 1 and upwards, and has 0 as the illegal 'null'
// reference. A null array will always refer to an empty array,
// because we insert an empty array at the first slot in the index
// list.
typedef int AIndex;

// Not all of these are used yet.
enum AFlags : int
  {
    None        = 0x00,
    Alive       = 0x01, // This reference is not deleted
    Const       = 0x02, // Constant data
    CanCollect  = 0x04, // Can be colleted by the GC
    RefCounted  = 0x08, // Is reference counted
    Marked      = 0x10, // Was marked in the last GC sweep
    Null        = 0x20, // Is the null array
  }

struct ArrayRef
{
  union
  {
    int[] iarr;
    float[] farr;
    dchar[] carr;
    AIndex[] aarr;
  }
  Flags!(AFlags) flags;

  uint elemSize; // Size of each element (in ints)

  AIndex getIndex()
  {
    return cast(AIndex)( Arrays.ArrayList.getIndex(this) );
  }

  // Array length, in terms of its element size
  uint length()
  {
    if(isNull) return 0;
    assert(elemSize != 0, "elemSize not set");
    assert(iarr.length % elemSize == 0, "array length not divisible by element size");
    return iarr.length / elemSize;
  }

  bool isAlive() { return flags.has(AFlags.Alive); }
  bool isConst() { return flags.has(AFlags.Const); }
  bool isNull() { return flags.has(AFlags.Null); }
}

Arrays arrays;

struct Arrays
{
  alias FreeList!(ArrayRef) ArrayList;

  private:
  ArrayList arrList;

  // Get a new array reference
  ArrayRef *createArray()
  {
    ArrayRef *ar = arrList.getNew();

    assert(!ar.isAlive);

    // Set the "alive" flag
    ar.flags.set(AFlags.Alive);

    assert(!ar.isNull);

    return ar;
  }

  // Put a reference back into the freelist
  void destroyArray(ArrayRef *ar)
  {
    assert(ar.isAlive);
    assert(!ar.isNull);
    assert(!ar.isConst);
    ar.flags.unset(AFlags.Alive);
    arrList.remove(ar);
  }

  public:

  // Set up this struct
  void initialize()
  {
    // Make sure index zero is valid and is an empty array. Set
    // more flags later.
    auto ar = createArray();
    ar.iarr = null;
    ar.flags.set(AFlags.Null);

    assert(ar.getIndex == 0);
  }

  // Get the reference to the empty array
  ArrayRef *getZero()
  {
    return getRef(cast(AIndex)0);
  }

  ArrayRef *createT(T)(T[] data)
  {
    static if(T.sizeof == 4) return create(cast(int[])data, 1);
    else static if(T.sizeof == 8) return create(cast(int[])data, 2);
    else static assert(0);
  }

  alias createT!(int) create;
  alias createT!(uint) create;
  alias createT!(long) create;
  alias createT!(ulong) create;
  alias createT!(float) create;
  alias createT!(double) create;
  alias createT!(dchar) create;
  alias createT!(AIndex) create;

  // Generic element size
  ArrayRef *create(int[] data, int size)
  {
    assert(size > 0);

    if(data.length == 0) return getZero();

    ArrayRef *ar = createArray();
    ar.iarr = data;
    ar.elemSize = size;

    if(data.length % size != 0)
      fail("Array length not divisible by element size");

    return ar;
  }

  ArrayRef *createConst(int[] data, int elem)
  {
    ArrayRef *arf = create(data, elem);
    arf.flags.set(AFlags.Const);
    return arf;
  }

  ArrayRef *getRef(AIndex index)
  {
    if(index < 0 || index >= getTotalArrays())
      fail("Invalid array reference: " ~ toString(cast(int)index));

    ArrayRef *arr = ArrayList.getNode(index);

    if(!arr.isAlive)
      fail("Dead array reference: " ~ toString(cast(int)index));

    assert(arr.getIndex() == index);

    if(index == 0) assert(arr.iarr.length == 0);

    return arr;
  }

  // Get the number of array references in use
  int getArrays()
  {
    return arrList.length();
  }

  // Get the total number of Array references ever allocated for the
  // free list.
  int getTotalArrays()
  {
    return ArrayList.totLength();
  }
}

// Create a multi-dimensional array of rank 'rank' and innermost data
// initialized to 'initval'. The array lengths are popped of the
// script stack.
void createMultiDimArray(int rank, int init[])
{
  if(rank <= 0 || rank >= 30)
    fail("Invalid array nesting number " ~ toString(rank));

  assert(init.length > 0);

  int[30] lenbuf;
  int[] lens = lenbuf[0..rank];
  int[] data; // All the elements + overhead data
  ulong totElem = 1; // Total number of elements. Set to 1 and
                     // multiplied with the length later.
  ulong totSize = 0; // Total size of data to allocate

  int[] currSlice; // Current slice of the data, used by getNext.

  // Get the next 'count' ints of data, in the form of a newly created
  // ArrayRef.
  ArrayRef *getNext(int count, int elemSize=1)
    {
      assert(count <= currSlice.length);

      int[] res = currSlice[0..count];

      currSlice = currSlice[count..$];

      return arrays.create(res, elemSize);
    }

  // Get the lengths, and calculate how much data we need. The first
  // length is the outermost wrapper, the last is the number of
  // actual elements in the innermost array wrapper.
  foreach(int i, ref int len; lens)
    {
      len = stack.popInt();

      // Do some sanity check on the length. The upper bound set here
      // is pretty arbitrary, we might enlarge it later.
      if(len <= 0 || len > 0x100000)
        fail("Invalid array length " ~ toString(len));

      // We could allow 0-length arrays here, but there's not much
      // point really.

      // Calculate in the element size in the last element
      if(i == lens.length-1) len *= init.length;

      // The total data is the cumulative value of totElem through all
      // iterations. For example, if we have a k*m*n array, we must
      // have k outer arrays, indexing a total of k*m subarrays,
      // indexing a total of k*m*n elements. The total data size,
      // assuming element sizes have been figured in, is
      // k + k*m + k*m*n.
      totElem *= len;
      totSize += totElem;
    }

  // Allocate all the elements + overhead (data for the lookup arrays)
  if(totSize)
    {
      assert(totElem >= 0 && totElem <= totSize);

      // Let's slap a 10 meg sanity check on the total data size
      if(totSize > 10*1024*1024)
        fail("Total array size is too large: " ~ toString(totSize));

      data.length = totSize;

      // Set currSlice to point to the entire data
      currSlice = data;
    }

  // Set up inner arrays recursively. This can be optimized heavily
  // later (removing recursion, moving if-tests out of loops, avoiding
  // double initialization, and so on.)
  void setupArray(int lenIndex, ArrayRef *arr)
    {
      // Length of arrays at this level
      int len = lens[lenIndex];

      // Loop through the previous level and create the arrays of this level
      foreach(ref AIndex ind; arr.aarr)
        {
          ArrayRef *narr;
          if(lenIndex == rank-1)
            // Remember to set the element size on the inner level
            narr = getNext(len, init.length);
          else
            narr = getNext(len);

          // Store the index or this array in the previous level
          ind = narr.getIndex();

          // Is this the innermost level?
          if(lenIndex == rank-1)
            {
              // If so, this is an array of elements. Initialize them.
              if(init.length == 1) narr.iarr[] = init[0];
              else if(init.length == 2) (cast(long[])narr.iarr)[] = *(cast(long*)init.ptr);
              else
                for(int i=0; i<lens[0]; i+=init.length)
                  arr.iarr[i..i+init.length] = init[];
            }
          else
            // If not, set up the indices in this array
            setupArray(lenIndex+1, narr);
        }
    }

  if(rank > 1)
    {
      // Create outer array and push it
      ArrayRef *arr = getNext(lens[0]);
      stack.pushArray(arr);

      // Recursively set up the sub-arrays
      setupArray(1, arr);
    }
  else
    {
      // Create outer array and push it. Element size has already been
      // multiplied into the length.
      ArrayRef *arr = getNext(lens[0], init.length);
      stack.pushArray(arr);

      // There is only one array level, so this IS the inner
      // array. Initialize the elements. Optimize for element sizes 1
      // and 2
      if(init.length == 1) arr.iarr[] = init[0];
      else if(init.length == 2) (cast(long[])arr.iarr)[] = *(cast(long*)init.ptr);
      else
        for(int i=0; i<lens[0]; i+=init.length)
          arr.iarr[i..i+init.length] = init[];
    }

  // Make sure we used all the data!
  assert(currSlice.length == 0);
}

// There's no phobos function that does unicode case insensitive
// string comparison, so let's make one ourselves. This can probably
// be optimized. toUniLower is in prinicple an expensive operation,
// but not so much if we assume most characters are ascii.
bool isUniCaseEqual(dchar[] a, dchar[] b)
{
  if(a.length != b.length) return false;
  foreach(int i, dchar ch; a)
    if(ch != b[i] && toUniLower(ch) != toUniLower(b[i]))
      return false;
  return true;
}
