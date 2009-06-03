/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (regions.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
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

module util.regions;

private import std.gc;
private import std.string;
private import std.c.stdlib;

private import monster.util.string;

alias std.c.stdlib.malloc malloc;

class RegionManagerException : Exception
{
  this(char[] msg, char[] name)
    { super( format("Memory Region Manager '%s': %s", name, msg ) ); }
}

// A resizable array using a region for memory allocation. These can
// safely be resized back and forth without wasting large amounts of
// memory.
class RegionBuffer(T)
{
 final:
 private:
  T[] buffer;
  T[] inUse;

  int reserve = 20;

  RegionManager reg;

 public:

  // Size is initial size, reserve gives an indicator of the minimum
  // amount to increase the array with at once.
  this(RegionManager m, int size = 0, int reserve = 0)
    {
      reg = m;
      if(reserve) this.reserve = reserve;
      if(size) alloc(size);
    }

  new(uint size, RegionManager r)
    {
      return r.allocate(size).ptr;
    }

  delete(void *p) { assert(0); }

  // Check if the buffer can hold 'size' more elements. If not,
  // increase it. NOTE: This works even if data = null, and inUse
  // is not. This allows us to make copy-on-resize slices.
  private void alloc(ulong size)
    {
      if(inUse.length + size <= buffer.length)
	{
	  inUse = buffer[0..inUse.length+size];
	  return;
	}

      // Allocate a new array, with more entries than requested
      buffer = reg.allocateT!(T)(buffer.length + size + reserve);

      // Copy the old data
      buffer[0..inUse.length] = inUse;

      // Set up the array in use
      inUse = buffer[0..(inUse.length+size)];
    }

  T opIndex(int i)
    {
      return inUse[i];
    }
  T opIndexAssign(T val, int i) { return inUse[i] = val; }

  RegionBuffer opSlice(int x, int y)
    {
      RegionBuffer b = new(reg) RegionBuffer(reg,0,reserve);
      b.inUse = inUse[x..y];
      return b;
    }

  RegionBuffer opSlice() { return opSlice(0,inUse.length); }

  RegionBuffer opCatAssign(T t)
    {
      alloc(1);
      inUse[$-1] = t;
      return this;
    }

  RegionBuffer opCatAssign(T[] t)
    {
      alloc(t.length);
      inUse[($-t.length)..$] = t;
      return this;
    }

  RegionBuffer opCatAssign(RegionBuffer t)
    {
      alloc(t.inUse.length);
      inUse[($-t.inUse.length)..$] = t.inUse;
      return this;
    }

  RegionBuffer opCat(T t) { return this.dup() ~= t; }
  RegionBuffer opCat(T[] t) { return this.dup() ~= t; }
  RegionBuffer opCat(RegionBuffer t) { return this.dup() ~= t; }

  RegionBuffer dup()
    {
      RegionBuffer b = new(reg) RegionBuffer(reg, inUse.length, reserve);
      b.inUse[] = inUse[];
      return b;
    }

  ulong length() { return inUse.length; }

  void length(ulong size)
    {
      // Grow array
      if(size > inUse.length) alloc(size - inUse.length);

      // Shrink array
      else inUse = inUse[0..size];
    }

  // For direct access
  T[] array() { return inUse; }
}

alias RegionBuffer!(char) RegionCharBuffer;
alias RegionBuffer!(int) RegionIntBuffer;

/*
 * Region manager, a mark-sweep memory manager. You use it to allocate
 * a lot of buffers, and when you are done with them you deallocate
 * them all in one fell sweep.
 *
 */
class RegionManager
{
 final:
 private:
  // Identifying name, used for error messages.
  char[] name;

  // Use a default buffer size of one meg. Might change later.
  const ulong defaultBufferSize = 1024*1024;

  // The size to use for new buffers.
  ulong bufferSize;

  // Current amount of space that is 'lost' in unused end-of-buffer
  // areas. Since we have proceeded to other buffers, this space will
  // remain unused until freeAll is called.
  ulong lost;

  ubyte[][] buffers; // Actual memory buffers
  void *gcRanges[]; // List of ranges added to gc

  ubyte[] left;	// Slice of what is left of the current buffer

  int currentBuffer; // Index into the next unused buffer
  int currentRange; // Index into the next unused gcRanges entry

  void fail(char[] msg)
    {
      throw new RegionManagerException(msg, name);
    }

  // We have run out of space, add a new buffer. I want this to be an
  // exception rather than the rule, the default bufferSize should be
  // large enough to prevent this from being necessary in most cases.
  void nextBuffer()
    {
      // We should never have to increase the number of buffers!
      if(currentBuffer >= buffers.length) fail("Out of buffers");

      // Set up the buffer (if it is not already allocated.)
      //buffers[currentBuffer].length = bufferSize;
      if(buffers[currentBuffer].length != bufferSize)
	{
	  assert(buffers[currentBuffer].length == 0);
	  ubyte *p = cast(ubyte*)malloc(bufferSize);
	  if(!p) fail("Malloc failed");
	  buffers[currentBuffer] = p[0..bufferSize];
	}

      // Remember the amount of space we just lost
      lost += left.length;

      // The entire buffer is available to us
      left = buffers[currentBuffer];

      // Point to the next unused buffer
      currentBuffer++;
    }

 public:

  this(char[] name = "", ulong bufferSize = defaultBufferSize)
    {
      this.name = name;
      this.bufferSize = bufferSize;

      // Pointers are cheap. Let's preallocate these arrays big enough
      // from the start. It's inflexible, but on purpose. We shouldn't
      // NEED to grow them later, so we don't.
      buffers.length = 100;
      gcRanges.length = 10000;

      freeAll();
    }

  ~this()
    {
      // Don't leave any loose ends dangeling for the GC.
      freeAll();

      // Kill everything
      foreach(ubyte[] arr; buffers)
	free(arr.ptr);

      delete buffers;
      delete gcRanges;
    }

  ulong getBufferSize() { return bufferSize; }

  // Allocates an array from the region.
  ubyte[] allocate(ulong size)
    {
      if(size > bufferSize)
	fail(format("Tried to allocate %d, but maximum allowed allocation size is %d",
		    size, bufferSize));

      // If the array cannot fit inside this buffer, get a new one.
      if(size > left.length) nextBuffer();

      ubyte[] ret = left[0..size];

      left = left[size..$];

      //writefln("Allocated %d, %d left in buffer", size, left.length);

      return ret;
    }

  // Allocate an array and add it to the GC as a root region. This
  // should be used for classes and other data that might contain
  // pointers / class references to GC-managed data.
  ubyte[] allocateGC(ulong size)
    {
      if(currentRange >= gcRanges.length)
	fail("No more available GC ranges");

      ubyte[] ret = allocate(size);

      // Add it to the GC
      void *p = ret.ptr;
      std.gc.addRange(p, p+ret.length);
      gcRanges[currentRange++] = p;

      return ret;
    }

  // Allocate an array of a specific type, eg. to allocate 4 ints, do
  // int[] array = allocateT!(int)(4);
  template allocateT(T)
    {
      T[] allocateT(ulong number)
	{
	  return cast(T[])allocate(number * T.sizeof);
	}
    }

  alias allocateT!(int) getInts;
  alias allocateT!(char) getString;

  template newT(T)
    {
      T* newT()
	{
	  return cast(T*)allocate(T.sizeof);
	}
    }

  template allocateGCT(T)
    {
      T[] allocateGCT(ulong number)
	{
	  return cast(T[])allocateGC(number * T.sizeof);
	}
    }

  // Copies an array of a given type
  template copyT(T)
    {
      T[] copyT(T[] input)
	{
	  T[] output = cast(T[]) allocate(input.length * T.sizeof);
	  output[] = input[];
	  return output;
	}
    }

  alias copyT!(int) copy;
  alias copyT!(char) copy;

  // Copies a string and ensures that it is null-terminated
  char[] copyz(char[] str)
    {
      char[] res = cast(char[]) allocate(str.length+1);
      res[$-1] = 0;
      res = res[0..$-1];
      res[] = str[];
      return res;
    }

  // Resets the region manager, but does not deallocate the
  // buffers. To do that, delete the object.
  void freeAll()
    {
      lost = 0;
      currentBuffer = 0;
      left = null;

      // Free the ranges from the GC's evil clutch.
      foreach(inout void *p; gcRanges[0..currentRange])
	if(p)
	  {
	    std.gc.removeRange(p);
	    p = null;
	  }

      currentRange = 0;
    }

  // Number of used buffers, including the current one
  ulong usedBuffers() { return currentBuffer; }

  // Total number of allocated buffers
  ulong totalBuffers()
    {
      ulong i;

      // Count number of allocated buffers
      while(i < buffers.length && buffers[i].length) i++;

      return i;
    }

  // Total number of allocated bytes
  ulong poolSize()
    {
      return bufferSize * totalBuffers();
    }

  // Total number of bytes that are unavailable for use. (They might
  // not be used, as such, if they are at the end of a buffer but the
  // next buffer is in use.)
  ulong usedSize()
    {
      return currentBuffer*bufferSize - left.length;
    }

  // The total size of data that the user has requested.
  ulong dataSize()
    {
      return usedSize() - lostSize();
    }

  // Number of lost bytes
  ulong lostSize()
    {
      return lost;
    }

  // Total amount of allocated space that is not used
  ulong wastedSize()
    {
      return poolSize() - dataSize();
    }

  // Give some general info and stats
  char[] toString()
    {
      return format("Memory Region Manager '%s':", name,
		    "\n  pool %s (%d blocks)", comma(poolSize), totalBuffers,
		    "\n  used %s", comma(usedSize),
		    "\n  data %s", comma(dataSize),
		    "\n  wasted %s (%.2f%%)", comma(wastedSize),
		                              poolSize()?100.0*wastedSize()/poolSize():0,
		    "\n  lost %s (%.2f%%)", comma(lost), usedSize()?100.0*lost/usedSize:0);
    }

  // Get a RegionBuffer of a given type
  template getBuffer(T)
    {
      RegionBuffer!(T) getBuffer(int size = 0, int reserve = 0)
	{
	  return new(this) RegionBuffer!(T)(this,size,reserve);
	}
    }

  alias getBuffer!(char) getCharBuffer;
  alias getBuffer!(int) getIntBuffer;
}

unittest
{
  RegionManager r = new RegionManager("UT", 100);

  // Test normal allocations first
  assert(r.poolSize == 0);
  assert(r.usedSize == 0);
  assert(r.dataSize == 0);
  assert(r.lostSize == 0);
  assert(r.wastedSize == 0);

  ubyte [] arr = r.allocate(30);
  void *p = arr.ptr;

  assert(p == r.buffers[0].ptr);
  assert(arr.length == 30);
  assert(r.poolSize == 100);
  assert(r.usedSize == 30);
  assert(r.dataSize == 30);
  assert(r.lostSize == 0);
  assert(r.wastedSize == 70);

  arr = r.allocate(70);
  assert(arr.ptr == p + 30);
  assert(arr.length == 70);
  assert(r.poolSize == 100);
  assert(r.usedSize == 100);
  assert(r.dataSize == 100);
  assert(r.lostSize == 0);
  assert(r.wastedSize == 0);

  // Overflow the buffer
  p = r.allocate(2).ptr;
  assert(p == r.buffers[1].ptr);
  assert(r.poolSize == 200);
  assert(r.usedSize == 102);
  assert(r.dataSize == 102);
  assert(r.lostSize == 0);
  assert(r.wastedSize == 98);

  // Overflow the buffer and leave lost space behind
  r.freeAll();
  assert(r.poolSize == 200);
  assert(r.usedSize == 0);
  assert(r.dataSize == 0);
  assert(r.lostSize == 0);
  assert(r.wastedSize == 200);

  r.allocate(1);
  r.allocate(100);
  assert(r.poolSize == 200);
  assert(r.usedSize == 200);
  assert(r.dataSize == 101);
  assert(r.lostSize == 99);
  assert(r.wastedSize == 99);

  // Try to allocate a buffer that is too large
  bool threw = false;
  try r.allocate(101);
  catch(RegionManagerException e)
    {
      threw = true;
    }
  assert(threw);

  // The object should still be in a valid state.

  // Try an allocation with roots
  assert(r.currentRange == 0);
  arr = r.allocateGC(50);
  assert(r.poolSize == 300);
  assert(r.usedSize == 250);
  assert(r.dataSize == 151);
  assert(r.lostSize == 99);
  assert(r.wastedSize == 149);
  assert(r.currentRange == 1);
  assert(r.gcRanges[0] == arr.ptr);

  int[] i1 = r.allocateGCT!(int)(10);
  assert(i1.length == 10);
  assert(r.poolSize == 300);
  assert(r.usedSize == 290);
  assert(r.dataSize == 191);
  assert(r.lostSize == 99);
  assert(r.currentRange == 2);
  assert(r.gcRanges[1] == i1.ptr);

  r.freeAll();
  assert(r.currentRange == 0);
  assert(r.poolSize == 300);
  assert(r.usedSize == 0);
  assert(r.dataSize == 0);
  assert(r.lostSize == 0);

  // Allocate some floats
  float[] fl = r.allocateT!(float)(24);
  assert(fl.length == 24);
  assert(r.poolSize == 300);
  assert(r.usedSize == 96);
  assert(r.dataSize == 96);
  assert(r.lostSize == 0);

  // Copy an array
  r.freeAll();
  char[] stat = "hello little guy";
  assert(r.dataSize == 0);
  char[] copy = r.copy(stat);
  assert(copy == stat);
  assert(copy.ptr != stat.ptr);
  copy[0] = 'a';
  copy[$-1] = 'a';
  assert(stat != copy);
  assert(stat == "hello little guy");
  assert(r.dataSize == stat.length);

  // Test copyz()
  r.freeAll();
  stat = "ABC";
  char *pp = cast(char*) r.copyz(stat).ptr;
  assert(pp[2] == 'C');
  assert(pp[3] == 0);
  copy = r.copyz(stat);
  assert(cast(char*)copy.ptr - pp == 4);
  assert(pp[4] == 'A');
  copy[0] = 'F';
  assert(pp[3] == 0);
  assert(pp[4] == 'F');

  // Test of the buffer function
  r.freeAll();
  RegionBuffer!(int) b = r.getBuffer!(int)();
  assert(b.inUse.length == 0);
  assert(b.reserve == 20);

  b.reserve = 5;

  assert(b.length == 0);
  b ~= 10;
  b ~= 13;

  assert(b.length == 2);
  assert(b[0] == 10);
  assert(b[1] == 13);
  assert(b.buffer.length == 6);
  p = b.buffer.ptr;

  b.length = 0;
  b.length = 6;
  assert(p == b.buffer.ptr);
  assert(b.length == 6);

  b.length = 3;
  assert(p == b.buffer.ptr);
  assert(b.length == 3);

  b[2] = 167;

  b.length = 7;
  assert(p != b.buffer.ptr); // The buffer was reallocated
  assert(b.length == 7);

  assert(b[2] == 167);

  i1 = new int[5];
  foreach(int v, inout int i; i1)
    i = v;

  p = b.buffer.ptr;
  RegionBuffer!(int) a = b ~ i1;
  assert(p != a.buffer.ptr); // A new buffer has been allocated
  assert(p == b.buffer.ptr); // B should be unchanged
  assert(a.length == b.length + i1.length);

  for(int i=0; i < b.length; i++)
    assert(a[i] == b[i]);

  for(int i=0; i < i1.length; i++)
    assert(a[i+7] == i);

  // Make sure the arrays are truly different
  a[5] = b[5] + 2;
  assert(a[5] != b[5]);

  // Make a slice
  a = b[2..5];
  assert(a.inUse.ptr == b.inUse.ptr+2);
  a[1] = 4;
  assert(a[1] == b[3]);
  b[3] = -12;
  assert(a[1] == b[3]);

  a.length = a.length + 1;
  assert(a.inUse.ptr != b.inUse.ptr+2);
  a[1] = 4;
  assert(a[1] != b[3]);
  b[3] = -12;
  assert(a[1] != b[3]);

  r.freeAll();
}
