/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (freelist.d) is part of the Monster script language
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

module monster.util.freelist;

import monster.util.list;
import monster.util.growarray;
import std.c.stdlib : malloc, free;

// A freelist for buffers of a given size. The 'size' template
// parameter gives the total requested size of the entire struct,
// including overhead.
struct BufferList(int size)
{
  // Calculate the 'overhead' size of the structs
  alias void* vp;
  static const junk = 2*vp.sizeof + int.sizeof;

  static union BuffData(int size)
  {
    static assert(size >= 2, "size must be at least 2");

    int[size/int.sizeof] ints;
    ubyte[size] bytes;
  }

  // Get the data sizes
  static const bytes = size - junk;
  static const ints = bytes / int.sizeof;

  static assert(bytes > 0, "size is too small");

  alias BuffData!(bytes) Value;
  alias Value* ValuePtr;
  static assert(Value.sizeof == bytes);

  alias LinkedList!(Value, NoAlloc) List;
  alias List.Node LNode;

  static struct BufferNode(int size)
    {
      LNode data;
      int index;
    }

  alias BufferNode!(bytes) Node;
  alias Node* NodePtr;
  static assert(Node.sizeof == size);

 private:

  // This is the array that does all the actual allocations. It is
  // used for quickly looking up indices. GrowArrays are designed to
  // grow dynamically without reallocation, while still being easily
  // indexed like a normal array.
  static GrowArray!(Node) array;

  // The freelist. This is shared between all template instances of
  // the same size.
  static List freeList;

  // The nodes belonging to THIS list instance
  List nodes;

  // Get a new node (move from freelist to node list)
  ValuePtr getNew()
    {
      // Is the freelist empty?
      if(freeList.length == 0)
	{
	  // Create a bunch of nodes and shove them into the freelist.
	  const makeSize = 50;

	  // Grow the growarray
	  uint len = array.length;
	  array.length = len + makeSize;

	  // Loop through the new nodes, number them, and insert them
	  // into freeList
	  for(int i=0; i < makeSize; i++)
	    {
	      NodePtr fn = array.getPtr(i+len);
	      fn.index = i + len;
	      freeList.insertNode(&fn.data);
	    }
	}

      // Move the first element from the freelist into the node list.
      auto node = freeList.getHead;
      freeList.removeNode(node);
      nodes.insertNodeFirst(node);

      // Return the value pointer. Since the value is always at the
      // begining of the Node struct, this is the same
      // pointer.
      return &node.value;
    }

  // Move a node back to the freelist ("delete" it)
  void remove(ValuePtr node)
    {
      nodes.removeNode(node);
      freeList.insertNodeFirst(node);
    }

 public:

  // Get the node corresponding to an index
  static ValuePtr getNode(int index)
    {
      return &array.getPtr(index).data.value;
    }

  // Get the index from a node
  static int getIndex(ValuePtr node)
    {
      return ( cast(Node*)node ).index;
    }

  uint length() { return nodes.length; }
  static uint totLength() { return array.length; }

  // Move the given node to another list
  ValuePtr moveTo(ref BufferList fl, ValuePtr node)
    {
      nodes.removeNode(node);
      fl.nodes.insertNodeFirst(node);
      return node;
    }

  // Get the first element in the list
  ValuePtr getHead() { return &nodes.getHead().value; }

  // Loop through the structs in this list
  int opApply(int delegate(ref Value) dg)
    {
      return nodes.opApply(dg);
    }

  int[] getInt(int isize)
    {
      assert(isize <= ints);

      return getNew().ints[0..isize];
    }

  void freeInt(int[] buf)
    {
      assert(buf.length <= ints);
      remove(cast(ValuePtr)buf.ptr);
    }

  void* get() { return getNew(); }
  void free(void* p) { remove(cast(ValuePtr)p); }
}

import std.stdio;

struct Buffers
{
  static:
  BufferList!(64) b64;
  BufferList!(128) b128;
  BufferList!(256) b256;
  BufferList!(768) b768;

  /*
  static this()
  {
    writefln("64: ints=%s bytes=%s", b64.ints, b64.bytes);
    writefln("128: ints=%s bytes=%s", b128.ints, b128.bytes);
    writefln("256: ints=%s bytes=%s", b256.ints, b256.bytes);
    writefln("768: ints=%s bytes=%s", b768.ints, b768.bytes);
  }
  */

  int[] getInt(uint size)
  {
    if(size <= b64.ints) return b64.getInt(size);
    else if(size <= b128.ints) return b128.getInt(size);
    else if(size <= b256.ints) return b256.getInt(size);
    else if(size <= b768.ints) return b768.getInt(size);
    // Too large for our lists - just use malloc
    else
      {
        writefln("WARNING: using malloc for %s ints (%s bytes)",
                 size, size*int.sizeof);
        return ( cast(int*)malloc(size*int.sizeof) )[0..size];
      }
  }

  void free(int[] buf)
  {
    uint size = buf.length;
    if(size <= b64.ints) b64.freeInt(buf);
    else if(size <= b128.ints) b128.freeInt(buf);
    else if(size <= b256.ints) b256.freeInt(buf);
    else if(size <= b768.ints) b768.freeInt(buf);
    else .free(buf.ptr);
  }
}

/* THIS DOESN'T WORK - because DMD is still stubborn with those
   template forwarding issues. Instead we'll just reuse the old
   freelist implementation below.

// A list that uses a freelist for allocation. It is built on top of
// BufferList.
struct FreeList(T)
{
 private:
  // For small sizes, pool together with existing lists.
  static if(T.sizeof <= 64) static const size = 64;
  else static if(T.sizeof <= 128) static const size = 128;
  else static if(T.sizeof <= 256) static const size = 256;
  // Just use the actual size, rounded up to the nearest 16
  else static if(T.sizeof % 16 == 0)
    const size = T.sizeof;
  else
    const size = T.sizeof + 16 - (T.sizeof%16);

  alias BufferList!(size) BuffList;
  BuffList buffer;

  alias BuffList.Value Value;
  alias BuffList.ValuePtr ValuePtr;

  static assert(T.sizeof <= BuffList.bytes);

 public:

  // Get a new node (move from freelist to node list)
  T* getNew()
    { return cast(T*) buffer.get(); }

  // Move a node back to the freelist ("delete" it)
  void remove(T* node)
    { buffer.free(node); }

  // Get the node corresponding to an index
  static T* getNode(int index)
    { return cast(T*) buffer.getNode(index); }

  // Get the index from a node
  static int getIndex(T *node)
    { return buffer.getIndex(cast(ValuePtr)node); }

  uint length() { return buffer.length(); }
  static uint totLength() { return buffer.totLength(); }

  // Move the given node to another list
  T* moveTo(ref FreeList fl, T* node)
    {
      auto vp = cast(ValuePtr) node;
      return cast(T*) buffer.moveTo(fl.buffer, vp);
    }

  // Get the first element in the list
  T* getHead() { return cast(T*) buffer.getHead(); }

  // Loop through the structs in this list
  int opApply(int delegate(ref T) dg)
    {
      auto dgc = cast(int delegate(ref Value)) dg;
      return nodes.opApply(dgc);
    }
}
*/

// This had to be moved outside FreeList to work around some
// irritating DMD template problems. (Can you say Aaargh!)
struct __FreeNode(T)
{
  _lstNode!(T) data;
  int index;
}

// A list that uses a freelist for allocation. Based on
// LinkedList. Very basic, only functions that are actually in use in
// my own code are implemented.
struct FreeList(T)
{
  alias LinkedList!(T, NoAlloc) TList;
  alias TList.Node TNode;

 private:

  alias __FreeNode!(T) _FreeNode;

  // This is the array that does all the actual allocations. It is
  // used for quickly looking up indices.
  static GrowArray!(_FreeNode) array;

  // The freelist. This is shared between all template instances of
  // the same type, as far as I know. DMD might have some strange
  // behavior that I am not aware of, but the worst case is that we
  // end up with multiple freelists, which is not the end of the world
  // (although slightly inefficient.)
  static TList freeList;

  // The nodes belonging to THIS list
  TList nodes;

 public:
  // Get a new node (move from freelist to node list)
  T* getNew()
    {
      // Is the freelist empty?
      if(freeList.length == 0)
	{
	  // Create a bunch of nodes and shove them into the freelist.
	  const makeSize = 100;

	  // Grow the growarray
	  uint len = array.length;
	  array.length = len + makeSize;

	  // Loop through the new nodes, number them, and insert them
	  // into freeList
	  for(int i=0; i < makeSize; i++)
	    {
	      _FreeNode *fn = array.getPtr(i+len);
	      fn.index = i + len;
	      freeList.insertNode(&fn.data);
	    }
	}

      // Move the first element from the freelist into the node list.
      auto node = freeList.getHead;
      freeList.removeNode(node);
      nodes.insertNodeFirst(node);

      // Return the value pointer. Since the value is always at the
      // begining of the Node struct, this is the same
      // pointer. LinkedList lets us choose if we want to use T* or
      // Node*.
      return &node.value;
    }

  // Get the node corresponding to an index
  static T* getNode(int index)
    {
      return &array.getPtr(index).data.value;
    }

  // Get the index from a node
  static int getIndex(T *node)
    {
      return ( cast(_FreeNode*)node ).index;
    }

  // Move a node back to the freelist ("delete" it)
  void remove(T* node)
    {
      nodes.removeNode(node);
      freeList.insertNodeFirst(node);
    }

  uint length() { return nodes.length; }
  static uint totLength() { return array.length; }

  // Move the given node to another list
  T* moveTo(ref FreeList fl, T* node)
    {
      nodes.removeNode(node);
      fl.nodes.insertNodeFirst(node);
      return node;
    }

  // Get the first element in the list
  T* getHead() { return &nodes.getHead().value; }

  // Loop through the structs in this list
  int opApply(int delegate(ref T) dg)
    {
      return nodes.opApply(dg);
    }
}
