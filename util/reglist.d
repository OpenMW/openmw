/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (reglist.d) is part of the OpenMW package.

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

module util.reglist;

import util.regions;

import std.string;

class RegionListException : Exception
{
  this(char[] msg)
    {
      super("RegionListException: " ~ msg);
    }
}

/*
 * Internal structure used below
 */

struct _aaNode(Value)
{
  Value value;

  _aaNode* next;	// Next node
  _aaNode* prev;	// Previous node
}

/*
 * Very simple linked list that uses a specified region for
 * allocation.
 */

struct RegionList(Value)
{
 private:
  alias _aaNode!(Value) Node;

  Node *head; // This is the head of the linked list (first element)
  Node *tail; // New nodes are inserted here
  uint totalNum; // Number of elements

  RegionManager reg;

  // Throw an exception
  void fail(char[] msg)
    {
      msg = format("RegionList!(%s) exception: %s", typeid(Value).toString, msg);
      throw new RegionListException(msg);
    }

 public:

  alias Node* Iterator;

  // Equivalent to calling remove() on all elements
  void clear()
    {
      head = tail = null;
      totalNum = 0;
    }

  // Reset the object and assign a new region manager
  void init(RegionManager reg)
    {
      clear();
      this.reg = reg;
    }

  Iterator insert(Value v)
    {
      Node* p = createNode();

      if(tail)
        {
	  // Insert node at the end of the list
          assert(head != null);
          tail.next = p;
        }
      else
        {
          // This is the first element to be inserted
          assert(head == null);
          head = p;
        }
      p.prev = tail;
      tail = p;

      p.value = v;

      return p;
    }

  void remove(Iterator p)
    {
      // Remove from the list
      if(p.next) p.next.prev = p.prev;
      else // We're the tail
        {
          assert(tail == p);
          tail = p.prev;
        }

      if(p.prev) p.prev.next = p.next;
      else // We're head
        {
          assert(head == p);
          head = p.next;
        }

      totalNum--;
    }

  // Create a new node and return it's pointer
  private Node* createNode()
    {
      Node *p = cast(Node*) reg.allocate(Node.sizeof).ptr;

      // Initialize pointers
      p.next = null;
      p.prev = null;

      totalNum++;

      return p;
    }

  // Loop through the nodes in the order they were inserted
  int opApply(int delegate(ref Value v) del)
    {
      Node *p = head;
      uint safeGuard = 0;
      while(p != null)
	{
	  assert(safeGuard++ < totalNum);
	  int i = del(p.value);
	  if(i) return i;
	  p = p.next;
	}
      return 0;
    }

  // Loop through the nodes in the order they were inserted
  int opApply(int delegate(ref int ind, ref Value v) del)
    {
      Node *p = head;
      int ind = 0;
      while(p != null)
	{
	  assert(ind < totalNum);
	  int i = del(ind, p.value);
	  ind++;
	  if(i) return i;
	  p = p.next;
	}
      return 0;
    }

  // Number of elements
  uint length() { return totalNum; }
}

/*
unittest
{
  RegionManager r = new RegionManager();

  RegionList!(float) ll;
  ll.reg = r;

  assert(ll.length == 0);
  ll.Iterator it = ll.insert(10.4);
  writefln(r);
  assert(ll.length == 1);
  ll.insert(23);
  it = ll.insert(6.3);
  ll.insert(-1000);

  writefln(r);
  assert(ll.length == 4);

  foreach(float f; ll) writefln(f);

  ll.remove(it);

  assert(ll.length == 3);

  assert(r.dataSize() == 12*4);

  foreach(int i, float f; ll) writefln(i, " ", f);
}
import std.stdio;
*/
