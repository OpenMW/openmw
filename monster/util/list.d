/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (list.d) is part of the Monster script language
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

module monster.util.list;

// Set this to enable some more extensive list checks. These will loop
// through the entire list on every insert and remove, so they are
// very slow for large lists. But they are very handy bug catchers
// when doing a little dirty list hacking, and they have saved me in
// the past.

// debug=slowcheck;

private import std.c.stdlib;
private import std.string;

typedef void GCAlloc;

alias malloc cmalloc;
alias free cfree;

class LinkedListException : Exception
{
  this(char[] msg)
    {
      super(msg);
    }
}

/*
 * Internal structure used by List
 */

align(1)
struct _lstNode(Value)
{
  // It is essential that the value is first in the struct. This
  // allows us to interchange pointers to the value with pointer to
  // the Node. This is done for convenience - allowing us to use the
  // value directly instead of using somePtr.value, This also
  // sidesteps the fact that DMD isn't very good with template forward
  // references, something that creates a lot of problems if we use
  // LinkedList.Iterator for everything (trust me on this.)
  Value value;

  _lstNode* getNext() { return next; }
  _lstNode* getPrev() { return prev; }

 private:
  _lstNode* next;	// Next node
  _lstNode* prev;	// Previous node
}

/*
 * This is a doubly linked list. It's not terribly advanced at the
 * moment, but I don't need any more functionality right now.
 *
 * Alloc must have the following members:
 *   void* alloc(uint size)
 *   void free(void*)
 *   bool autoinit; // True if alloc automatically sets memory to zero.
 */

// Example allocator using malloc and free:
struct Malloc
{
  static const bool autoinit = false; // malloc does not initialize memory
  static const bool usefree = true; // We must call free() to release memory
  static void* alloc(uint size) { return cmalloc(size); }
  static void free(void* p) { cfree(p); }
}

// A null allocator. Use if you only intend to move nodes into and out
// of the list, not to allocate them. Useful for a freelist, for
// example.
struct NoAlloc
{
  static const bool autoinit = false;
  static const bool usefree = true;
  static void *alloc(uint size) { assert(0, "NoAlloc.alloc not allowed"); }
  static void free(void *p) { assert(0, "NoAlloc.free not allowed"); }
}

struct LinkedList(Value, alias Alloc = GCAlloc)
{
  alias _lstNode!(Value) Node;

 private:

  Node *head; // This is the head of the linked list (first element)
  Node *tail; // New nodes are inserted here
  uint totalNum; // Number of elements

  // Determine if the allocator automatically initializes memory
  static if(is(Alloc == GCAlloc))
    static const bool autoinit = true;
  else static if(Alloc.autoinit)
    static const bool autoinit = true;
  else
    static const bool autoinit = false;

  // Determine if we have to manually free memory
  static if(is(Alloc == GCAlloc))
    static const bool usefree = false;
  else static if(Alloc.usefree)
    static const bool usefree = true;
  else
    static const bool usefree = false;

  // Throw an exception
  void fail(char[] msg)
    {
      msg = format("LinkedList!(%s) exception: %s", typeid(Value).toString, msg);
      throw new LinkedListException(msg);
    }

 public:

  // Whenever you find a bug that creates an invalid state, put it in
  // here so we can safeguard against regressions

  invariant()
    {
      if(head != null || tail != null || totalNum != 0)
	{
	  assert(head != null);
	  assert(tail != null);
	  assert(totalNum != 0);

	  assert(head.prev == null);
	  assert(tail.next == null);
	}
    }

  alias Node* Iterator;

  // Simply reset all pointers and variables, losing any nodes
  // present.
  void reset()
    {
      head = tail = null;
      totalNum = 0;
    }

  // Go through the list and delete all nodes
  void deleteAll()
    {
      // If there is no need to free objects, then deleteAll() is
      // equivalent to reset().
      static if(usefree)
	{
	  // Loop through the list and delete everything
	  Node *p = head;
	  while(p != null)
	    {
	      Node *next = p.next;
	      Alloc.free(p);
	      p = next;
	    }
	}
      reset();
    }

  Iterator getHead() { return head; }
  Iterator getTail() { return tail; }

  // Check if the given iterator is part of the list
  bool hasIterator(Node *v)
    {
      Node* p = head;

      while(p != null)
	{
	  if(p == v)
	    {
	      assert(length >= 1);
	      return true;
	    }
	  p = p.next;
	}
      return false;
    }

  // Insert a value at the end of the list.
  alias insert insertLast;
  Iterator insert(Value v)
    {
      Node *p = createNode();
      p.value = v;
      return insertNode(p);
    }
  // Also allow ~= syntax for this
  Iterator opCatAssign(Value v) { return insert(v); }

  // Insert an existing node at the end of the list. The insertNode*()
  // variants along with removeNode() are useful for removing and
  // reinserting nodes without allocating more memory. This can for
  // example be used for free lists and similar constructions. In
  // other words, you can use these to move elements from one list to
  // another.
  alias insertNode insertNodeLast;
  Iterator insertNode(Node *p)
    in
    {
      //debug(slowcheck)
        assert(!hasIterator(p), "inserNode: Node is already in the list");
    }
  body
    {
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
      p.next = null;

      totalNum++;

      return p;
    }
  // The Value* variants of the node functions work the same way as
  // their Iterator (Node*) versions. The pointers are the same, they
  // just need to be recast.
  Value* insertNode(Value *p)
    { return &insertNode( cast(Node*)p ).value; }

  // Beginning of the list
  Iterator insertFirst(Value v)
    {
      Node *p = createNode();
      p.value = v;
      return insertNodeFirst(p);
    }

  Iterator insertNodeFirst(Node *p)
    in
    {
      debug(slowcheck)
        assert(!hasIterator(p), "inserNodeFirst: Node is already in the list");
    }
    body
    {
      if(head)
        {
	  // Insert node at the beginning of the list
          assert(tail != null);
          head.prev = p;
        }
      else
        {
          // This is the first element to be inserted
          assert(tail == null);
          tail = p;
        }
      p.next = head;
      head = p;
      p.prev = null;

      totalNum++;

      return p;
    }
  Value* insertNodeFirst(Value *p)
    { return &insertNodeFirst( cast(Node*)p ).value; }

  // Insert after a given element
  Iterator insertAfter(Iterator i, Value v)
    {
      Node *p = createNode();
      p.value = v;
      return insertNodeAfter(i, p);
    }

  // Insert p after i
  Iterator insertNodeAfter(Iterator i, Node *p)
    in
    {
      //debug(slowcheck)
        {
          assert(!hasIterator(p), "inserNodeAfter: Node is already in the list");
          assert(hasIterator(i), "insertNodeAfter(): element i not part of the list");
        }
    }
  body
    {
      // If i is the last element, then insertNodeLast already does a
      // stellar job of inserting
      if(i == tail)
	return insertNodeLast(p);	

      // Make p point to the right elements
      p.next = i.next;
      p.prev = i;

      // Random consistency check
      assert(i == i.next.prev);

      // Make the right elements point to p
      i.next = p;
      assert(p.next != null);
      p.next.prev = p;

      totalNum++;

      return p;
    }
  // Insert p after i
  Value* insertNodeAfter(Value* p, Value* i)
    { return &insertNodeAfter( cast(Node*)p, cast(Node*)i ).value; }


  // Insert value v before i
  Iterator insertBefore(Iterator i, Value v)
    {
      Node *p = createNode();
      p.value = v;
      return insertNodeBefore(i, p);
    }

  // Insert p before i
  Iterator insertNodeBefore(Iterator i, Node *p)
    in
    {
      //debug(slowcheck)
        {
          assert(!hasIterator(p), "inserNodeBefore: Node is already in the list");
          assert(hasIterator(i), "insertBefore(): element not part of the list");
        }
    }
    body
    {
      // If i is the first, just insert at the beginning
      if(i==head) return insertNodeFirst(p);

      // If I mess it up, an assertion failure is easier to debug than
      // a segfault.
      assert(i.prev != null);

      // Reuse insertAfter instead of reinventing the wheel
      return insertNodeAfter(i.prev, p);
    }
  // Insert p before i
  Value* insertNodeBefore(Value* p, Value* i)
    { return &insertNodeBefore( cast(Node*)p, cast(Node*)i ).value; }


  // Swap position of element a and b
  void swap(Iterator a, Iterator b)
    in
    {
      //debug(slowcheck)
        assert(hasIterator(a) && hasIterator(b),
               "swap(a,b): both elements must be in the list");
    }
  body
    {
      Iterator tmp;

      // Handle special cases first

      // The same element? Do nothing.
      if(a==b) return;

      // Are they next to each other?
      if(b.next == a)
	{
	  // Swap it so we have a before b, then handle it below.
	  assert(a.prev == b);
	  tmp = a;
	  a = b;
	  b = tmp;
	}

      // Point a.prev to b
      if(a.prev) a.prev.next = b;
      else
	{
	  assert(head == a);
	  head = b;
	}
      // Point to b.next a
      if(b.next) b.next.prev = a;
      else
	{
	  assert(tail == b);
	  tail = a;
	}

      // From this point on, if a is next to b it must be handled as a
      // special case. We have already swapped them above so that a is
      // before b.
      if(a.next == b)
	{
	  assert(b.prev == a);

	  // Assign outer pointers
	  b.prev = a.prev;
	  a.next = b.next;

	  // Assign inner pointers
	  a.prev = b;
	  b.next = a;
	  return;
	}

      // If a is NOT next to b, continue the pointer orgy.

      // Point a.next to b
      if(a.next) a.next.prev = b;
      else
	{
	  assert(tail == a);
	  tail = b;
	}

      if(b.prev) b.prev.next = a;
      else
	{
	  assert(head == b);
	  head = a;
	}

      // Finally, swap a and b's internal pointers
      tmp = a.next;
      a.next = b.next;
      b.next = tmp;

      tmp = a.prev;
      a.prev = b.prev;
      b.prev = tmp;
    }
  void swap(Value* a, Value* b)
    { swap( cast(Node*)a, cast(Node*)b ); }

  // Remove a node from the list and delete it
  void remove(Iterator p)
    {
      removeNode(p);
      deleteNode(p);
    }

  // Just remove the node from the list, do not delete it.
  void removeNode(Iterator p)
    in
    {
      //debug(slowcheck)
        assert(hasIterator(p), "remove(): element not part of the list");
    }
  body
    {
      // Remove from the list
      if(p.next)
	{
	  p.next.prev = p.prev;

	  // Make sure we are NOT tail
	  assert(tail != p);
	}
      else // We're the tail
        {
          assert(tail == p);
          tail = p.prev;
        }

      if(p.prev)
	{
	  p.prev.next = p.next;

	  // We are NOT the head, since we have a previous element
	  assert(head != p);
	}
      else // We're head
        {
          assert(head == p);
          head = p.next;
        }

      totalNum--;
    }
  void removeNode(Value *v)
    { removeNode( cast(Iterator)v ); }

  // Free a node
  static private void deleteNode(Node *p)
    {
      // For the GC, just release the
      // pointer into the wild.
      static if(usefree) Alloc.free(p);
    }

  // Create a new node and return it's pointer. TODO: Make this
  // static, and increase totalNum in the insert methods instead.
  static private Node* createNode()
    {
      static if(is(Alloc == GCAlloc))
        Node *p = new Node;
      else
        Node *p = cast(Node*)Alloc.alloc(Node.sizeof);      

      // Initialize next pointers
      static if(!autoinit)
	{
	  p.next = null;
	  p.prev = null;
	}

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

  char[] toString()
    {
      char[] res = "[";
      foreach(int i, Value v; *this)
	{
	  if(i < totalNum-1) res ~= format(" %s,", v);
	  else res ~= format(" %s ]", v);
	}
      return res;
    }
}

/* This test is NOT very complete */
unittest
{
  LinkedList!(float) ll;

  assert(ll.length == 0);
  ll.Iterator it = ll.insert(10.4);
  assert(ll.length == 1);
  ll.insert(23);
  it = ll.insert(6.3);
  ll.insert(-1000);

  assert(ll.length == 4);

  //foreach(float f; ll) writefln(f);

  ll.remove(it);

  assert(ll.length == 3);

  ll.reset();

  assert(ll.length == 0);

  //foreach(int i, float f; ll) writefln(i, " ", f);
}
//import std.stdio;

// Array allocator. TODO: Put this and Malloc in their own place,
// extend list to be the same quality as aa.d and make a system out of
// it. Make some better unit tests.
struct ArrAlloc
{
  ubyte[] data;
  uint pos;

  void reset() { pos = 0; }

  const bool autoinit = false;
  const bool usefree = false;

  void* alloc(uint size)
  {
    if(pos+size > data.length)
      data.length = pos+size+30;

    void * ptr = &data[pos];

    pos += size;

    return ptr;
  }

  void free(void* p) { }
}
