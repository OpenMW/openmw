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

// TODO: Create unittests

import monster.util.list;
import monster.util.growarray;

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

  /*
  static struct _FreeNode
  {
    TNode data;
    int index;
  }
  */
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
