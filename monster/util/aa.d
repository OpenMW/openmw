/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (aa.d) is part of the Monster script language
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

module monster.util.aa;

private import std.string;
private import std.c.stdlib;

alias malloc cmalloc;
alias free cfree;

typedef void GCAlloc;
typedef void DefHash;

class HashTableException : Exception
{
  this(char[] msg)
    {
      super(msg);
    }
}

/*
 * Internal structure used by HashTable
 */

struct _aaNode(Key, Value)
{
  uint hash;		// Hash of this value
  Key key;
  Value value;

  _aaNode* next;	// Next node in current bucket
  _aaNode* prev;	// Previous node

  _aaNode* nextCont;	// Next node in the entire container
  _aaNode* prevCont;	// Previous node
}

/*
 * This hash table is actually a doubly linked list with added lookup
 * capabilities. I could rewrite this to use LinkedList.
 *
 * The key type is assumed to be inexpensive to copy. For large key
 * types use pointers and a custom hasher. Large value types will work
 * fine, but you should probably use inList, getPtr and insertEdit for
 * most lookup operations, since these return pointers instead of by
 * value.
 *
 * Alloc must have the following members:
 *   void* alloc(uint size)
 *   void free(void*)
 *   bool autoinit; // True if alloc automatically sets memory to zero.
 *
 * Example allocator using malloc and free:
 *
 * struct Malloc
 * {
 *   static const bool autoinit = false; // malloc does not initialize memory
 *   static void* alloc(uint size) { return std.c.stdlib.malloc(size); }
 *   static void free(void* p) { std.c.stdlib.free(p); }
 * }
 *
 *
 * Hash must contain:
 *   uint hash(Key k)
 *   int isEqual(Key a, Key b)
 *
 * Example integer hasher that ignores sign (ie. treats n and -n as
 * the same number):
 *
 * struct IgnoreSign
 * {
 *   static uint hash(int i) { return i<0 ? -i : i; }
 *   static int isEqual(int i, int j) { return (i==j) || (i==-j); }
 * }
 *
 * The keyFormat is used when constructing error messages. If set to
 * true, it is assumed that format("%s", key) will work. If set to
 * false, no attempt is made at converting the key to a string.
 *
 * Notice that the Hash functions are type specific, while the Alloc
 * ones are not.
 */

struct HashTable(Key, Value, Alloc = GCAlloc, Hash = DefHash,
		 bool keyFormat = true)
{
 private:
  alias _aaNode!(Key, Value) Node;

  Node *head; // Nodes form a linked list, this is the head.
  Node *tail; // New nodes are inserted here

  Node* array[]; // The hash table array

  uint bitmask; // The array size is a power of two, we perform a
		// bitwise AND between the hash and this bitmask to
		// look up in the array.

  uint totalNum;// Number of nodes currently in container

  // What size to choose if the user doesn't specify one.
  static const int defaultSize = 500;

  // Max allowed array size. 25 bits corresponds to about 34 million
  // entries.
  static const int maxBit = 25;
  static const uint maxSize = 1 << maxBit;

  // Minimum allowed size. 6 bits corresponds to 64 entries.
  static const int minBit = 6;

  // Assumed maximum bucket size, used as a safeguard against infinite
  // loops.
  static const int bucketMax = 100000;

  // Determine if the allocator automatically initializes memory
  static if(is(Alloc == GCAlloc))
    static const bool autoinit = true;
  else static if(Alloc.autoinit)
    static const bool autoinit = true;
  else
    static const bool autoinit = false;

  // Throw an exception
  void fail(char[] msg)
    {
      msg = format("HashTable!(%s,%s) exception: %s",
		   typeid(Key).toString, typeid(Value).toString, msg);
      throw new HashTableException(msg);
    }

  void fail(char[] msg, Key k)
    {
      static if(keyFormat) fail(format(msg, k));
      // Ignore key if we cannot display it
      else fail(format(msg, "(unshowable)"));
    }

 public:

  void reset()
    {
      // This stopped working in DMD 1.032 and later versions.
      //*this = typeof(*this).init;
      // But this seems to do the trick
      *this = (HashTable!(Key, Value, Alloc, Hash, keyFormat)).init;
    }

  // Returns number of buckets of each size. Mostly used for testing
  // efficiency of hash functions.
  int[] getStats()
    {
      int[] res;

      foreach(Node *n; array)
	{
	  // Get bucket size
	  uint size = 0;
	  while(n != null)
	    {
	      size++;
	      n = n.next;
	    }

	  if(size >= res.length) res.length = size+1;

	  // Add this bucket size to the stat
	  res[size]++;
	}
      return res;
    }
  //*/

  // Loop through all values and check that the stored hash values are
  // correct. Mostly used for debugging.
  //
  // For example, a common problem is to use a string as a key, and
  // then change the string afterwards. The HashTable does not store
  // the string contents (only the reference char[]), so if you change
  // the string contents later then there will be a mismatch between
  // the key and the hashed value. This function will detect the
  // problem.
  void validate()
    {
      Node *p = head;
      uint safeGuard = 0, hsh;
      while(p != null)
	{
	  assert(safeGuard++ < totalNum);
	  
	  static if(is(Hash == DefHash)) hsh = typeid(Key).getHash(&p.key);
	  else hsh = Hash.hash(p.key);

	  if(hsh != p.hash)
	    fail("Validation failed");// on key %s", p.key);

	  p = p.nextCont;
	}
    }

  // Look up a value. If found return true and set v to value,
  // otherwise return false.
  bool inList(Key k, ref Value v)
    {
      Node *p = lookupKey(k);
      if(p) v = p.value;
      return p != null;
    }

  // Look up a value. If found return true, otherwise false.
  bool inList(Key k) { return lookupKey(k) != null; }

  // Look up a value. If found return pointer, otherwise null.
  Value* lookup(Key k)
    {
      Node *p = lookupKey(k);
      if(p) return &p.value;
      return null;
    }

  // Get a value, throw an exception if it does not exist
  Value get(Key k)
    {
      Node *p = lookupKey(k);
      if(!p) fail("Cannot get key '%s', not found", k);
      return p.value;
    }
  Value opIndex(Key k) { return get(k); }

  // Get a pointer to a value, throw an exception if it does not exist
  Value* getPtr(Key k)
    {
      Node *p = lookupKey(k);
      if(!p) fail("Cannot get key '%s', not found", k);
      return &p.value;
    }

  // Get a value, insert a new if it doesn't exist. This is handy for
  // situations where the inserted values might or might not exist,
  // and you don't want to overwrite the existing value.
  Value get(Key k, Value def)
    {
      Node *p;
      uint hash;
      if(!lookupKey(k,hash,p))
	{
	  // Insert a node
	  p = insertNode(hash,p);
	  p.key = k;
	  p.value = def;
	}

      return p.value;
    }
  Value opIndex(Key k, Value v) { return get(k, v); }

  // Get a value. If it doesn't exist, insert a new one and call
  // apply(). Used in the same situation as get(Key, Value) but when
  // setting up the new value is expensive or otherwise to be avoided
  // when not needed. Handy for classes or when you wish to do
  // in-place editing. See also insertEdit.
  Value get(Key k, void delegate(ref Value v) apply)
    {
      Node *p;
      uint hash;
      if(!lookupKey(k,hash,p))
	{
	  // Insert a node and call apply
	  p = insertNode(hash,p);
	  p.key = k;
	  apply(p.value);
	}

      return p.value;
    }

  // Gets the stored key associated with the given key. This seemingly
  // useless function is handy for custom hashers that collapse
  // several values into one, for example the case insensitive string
  // hasher. getKey will return the key as it was originally inserted
  // into the list. In the case of reference types (such as arrays) it
  // can also be used for referencing the original data.
  Key getKey(Key k)
    {
      Node *p = lookupKey(k);
      if(!p) fail("Cannot get key '%s', not found", k);
      return p.key;
    }

  // Insert a new value, replace it if it already exists. Returns v.
  Value insert(Key k, Value v)
    {
      Node *p;
      uint hash;
      if(lookupKey(k, hash, p))
	// Key already existed, overwrite value.
	p.value = v;
      else
	{
	  // Insert a new node
	  p = insertNode(hash, p);
	  p.key = k;
	  p.value = v;
	}

      return v;
    }
  Value opIndexAssign(Value v, Key k) { return insert(k, v); }

  // Get a pointer to value of given key, or insert a new. Useful for
  // large value types when you want to avoid copying the value around
  // needlessly. Also useful if you want to do in-place
  // editing. Returns true if a new value was inserted.
  bool insertEdit(Key k, out Value *ptr)
    {
      Node *p;
      uint hash;
      if(!lookupKey(k,hash,p))
	{
	  // Insert it
	  p = insertNode(hash,p);
	  p.key = k;
	  ptr = &p.value;
	  return true;
	}
      ptr = &p.value;
      return false;
    }

  // TODO: No unittests for *ifReplace and replace yet

  // Insert a value. Return true if it replaced an existing value,
  // otherwise false.
  bool ifReplace(Key k, Value v)
    {
      Node *p;
      uint hash;
      if(!lookupKey(k,hash,p))
        {
          // Insert node
          p = insertNode(hash,p);
          p.key = k;
          p.value = v;
          return false; // We did not replace anything, return false
        }
      // A node was already found
      p.value = v;
      return true; // We replaced a value
    }

  // Replace an existing value with v, return the old value. Fail if
  // no value existed.
  Value replace(Key k, Value v)
    {
      Node *p = lookupKey(k);
      if(p)
        {
          Value old = p.value;
          p.value = v;
          return old;
        }
      fail("Failed to replace key '%s', not found", k);
      assert(0);
    }

  // Remove key, return value. If key does not exist, throw an
  // exception.
  Value remove(Key k)
    {
      Value v;
      if(!ifRemove(k, v))
	fail("Cannot remove key '%s', key not found", k);

      return v;
    }

  // Remove a key if it exists. If it was removed, return true,
  // otherwise false.
  bool ifRemove(Key k)
    {
      Value v;
      return ifRemove(k, v);
    }

  // Remove a key if it exists. If it existed, returns true and places
  // removed value in v. If not, return false;
  bool ifRemove(Key k, ref Value v)
    {
      Node *p = lookupKey(k);
      if(!p) return false;

      v = p.value;
      removeNode(p);
      return true;
    }

  // Loop through the nodes in the order they were inserted
  int opApply(int delegate(ref Key k, ref Value v) del)
    {
      Node *p = head;
      uint safeGuard = 0;
      while(p != null)
	{
	  assert(safeGuard++ < totalNum);
	  int i = del(p.key, p.value);
	  if(i) return i;
	  p = p.nextCont;
	}
      return 0;
    }

  // Same as above, but do not take Key as a parameter.
  int opApply(int delegate(ref Value v) del)
    {
      Node *p = head;
      uint safeGuard = 0;
      while(p != null)
	{
	  assert(safeGuard++ < totalNum);
	  int i = del(p.value);
	  if(i) return i;
	  p = p.nextCont;
	}
      return 0;
    }

  // Number of elements
  uint length() { return totalNum; }

  // Table size
  uint tabSize() { return array.length; }

  // Rehash the array, ie. resize the array and reinsert the
  // members. The parameter gives the requested number of elements. If
  // it is zero (or omitted), current number of elements is used
  // instead. This function never shrinks the array, use rehashShrink
  // for that.
  void rehash(uint size = 0)
    {
      if(size == 0) size = totalNum;

      // Do we need a rehash?
      if(size > array.length)
	// Let's do it.
	rehashShrink(size);
    }

  // Same as rehash, but allows the array to shrink
  void rehashShrink(uint size = 0)
    {
      if(size == 0) size = totalNum;

      killArray();
      allocArray(size);

      Node *p = head;
      uint safeGuard = 0;
      while(p != null)
	{
	  assert(safeGuard++ < totalNum);
	  rehashNode(p);
	  p = p.nextCont;
	}
    }

  // Place a node in the table. Assumes p.hash to be set. Does no
  // equality check and does not touch the main linked list.
  private void rehashNode(Node *p)
    {
      Node *n = array[p.hash & bitmask];
      int safeGuard = 0;

      p.next = null;
      //p.top = p.bottom = null;

      if(n == null)
	{
	  // We're all alone in this cold empty bucket
	  array[p.hash & bitmask] = p;
	  p.prev = null;
	  return;
	}

      // Bucket is occupied

      // Find the last element
      while(n.next != null)
	{
	  n = n.next;
	  assert(safeGuard++ < bucketMax);
	}

      // Place p at the end of the list
      p.prev = n;
      assert(n.next == null);
      n.next = p;
    }

  private void removeNode(Node *p)
    {
      // Remove from bucket
      if(p.next) p.next.prev = p.prev;
      if(p.prev) p.prev.next = p.next;
      else // We're at the start of the bucket, remove from hash table
	array[p.hash & bitmask] = p.next;

      // Remove from main list
      if(p.nextCont) p.nextCont.prevCont = p.prevCont;
      else // We're the tail
	{
	  assert(tail == p);
	  tail = p.prevCont;
	}
      if(p.prevCont) p.prevCont.nextCont = p.nextCont;
      else // We're head
	{
	  assert(head == p);
	  head = p.nextCont;
	}

      // Finally, delete the node. For the GC, just release the
      // pointer into the wild.
      static if(!is(Alloc == GCAlloc)) Alloc.free(p);

      totalNum--;
    }

  // Allocate a new node and inserts it into the main list. Places it
  // in the hash array as a successor of the given parent node. If
  // parent is null, insert directly into the array using the given
  // hash. Returns the newly inserted node.
  private Node *insertNode(uint hash, Node *parent)
    {
      // First, make the new node.
      static if(is(Alloc == GCAlloc))
	Node *p = new Node;
      else
	Node *p = cast(Node*)Alloc.alloc(Node.sizeof);

      p.hash = hash;

      // Is the bucket already occupied?
      if(parent)
	{
	  parent.next = p;
	  p.prev = parent;
	}
      else
	{
	  // Nope, start our very own bucket.
	  assert(array[hash & bitmask] == null);
	  array[hash & bitmask] = p;
	  static if(!autoinit) p.prev = null;
	}

      // Initialize pointers
      static if(!autoinit)
	{
	  p.next = null;
	  p.nextCont = null;
	}

      // Insert node into the main linked list
      if(tail)
	{
	  assert(head != null);
	  tail.nextCont = p;
	  p.prevCont = tail;
	}
      else
	{
	  // This is the first element to be inserted
	  assert(head == null);
	  head = p;
	  static if(!autoinit) p.prevCont = null;
	}

      tail = p;

      totalNum++;

      // I thiiiink it's time to rehash
      if(totalNum > 5*array.length)
	rehash();

      return p;
    }

  /* Looks up k in the hash table. Returns Node pointer if found, null
   * otherwise. This is identical in function to the other lookupKey
   * below, except that it returns less information and uses
   * marginally fewer instructions.
   */
  private Node* lookupKey(Key k)
    {
      // Check if the array is initialized.
      if(!array.length) return null;

      static if(is(Hash == DefHash))
	// Use TypeInfo.getHash to hash the key
	uint hsh = typeid(Key).getHash(&k);
      else
	// Use provided hash function
	uint hsh = Hash.hash(k);

      // Look up in the table
      Node* p = array[hsh & bitmask];

      // Search the bucket
      int safeGuard = 0;
      while(p)
	{
	  assert(safeGuard++ < bucketMax);

	  // First check if the hashes match
	  if(hsh == p.hash)
	    {
	      // They did, check if this is the correct one
	      static if(is(Hash == DefHash))
		{
		  // Use type specific compare
		  // This doesn't call opEquals. Fixed.
		  //if(!typeid(Key).compare(&k, &p.key)) return p;
		  if(k == p.key) return p;
		}
	      else
		{
		  // Use supplied compare
		  if(Hash.isEqual(k, p.key)) return p;
		}
	    }

	  // Next element
	  p = p.next;
	}

      // End of the list, no node found
      return null;
    }

  /* Looks up k in the hash table. This is used internally in
   * conjuntion with insertNode(). It has three outcomes:
   *
   * 1 - Key was found. Returns true. Ptr points to corresponding
   *     Node.
   *
   * 2 - Key was not found, but bucket is not empty. Returns
   *     false. Ptr points to last node in bucket list.
   *
   * 3 - Key was not found, bucket is empty. Returns false, ptr is
   *     null.
   *
   * Hash is in any case set to the correct hashed value of k.
   */
  private bool lookupKey(Key k, ref uint hash, ref Node* ptr)
    {
      // Make sure the array is initialized. We do this here instead
      // of in insertNode, because this function is always called
      // first anyway.
      if(!array.length) allocArray(defaultSize);

      static if(is(Hash == DefHash))
	// Use TypeInfo.getHash to hash the key
	uint hsh = typeid(Key).getHash(&k);
      else
	// Use provided hash function
	uint hsh = Hash.hash(k);

      // Return the value
      hash = hsh;

      // Look up in the table
      Node* p = array[hsh & bitmask];

      // Bucket is empty
      if(p == null)
	{
	  ptr = null;
	  return false;
	}

      // Search the bucket
      int safeGuard = 0;
      while(true)
	{
	  assert(safeGuard++ < bucketMax);

	  // First check if the hashes match
	  if(hsh == p.hash)
	    {
	      // They did, check if this is the One True Node
	      static if(is(Hash == DefHash))
		{
		  // Use type specific compare
		  //if(!typeid(Key).compare(&k, &p.key))
		  if(k == p.key)
		    {
		      ptr = p;
		      return true;
		    }
		}
	      else
		{
		  // Use supplied compare
		  if(Hash.isEqual(k, p.key))
		    {
		      ptr = p;
		      return true;
		    }
		}
	    }

	  // Break of at the end of the list
	  if(p.next == null) break;

	  p = p.next;
	}

      // Node was not found
      ptr = p;
      return false;
    }

  // Deallocates the hash table. Does not touch the nodes.
  private void killArray()
    {
      if(array == null) return;

      static if(!is(Alloc == GCAlloc))
	Alloc.free(array.ptr);

      // With the GC we just leave it dangeling in the wind.
      array = null;
    }

  // Allocates a new hash array, the old pointer is overwritten. The
  // parameter gives the requested number of elements, the actual
  // size will most likely be larger, since we must use a power of 2.
  private void allocArray(uint number)
  out
    {
      assert( array.length == bitmask+1 );
      assert( array.length != 0 );
    }
  body
    {
      if(number > maxSize) number = maxSize;

      for(uint highBit = minBit; highBit < maxBit+1; highBit++)
	{
	  uint len = 1 << highBit;
	  if(number < len)
	    {
	      static if(is(Alloc == GCAlloc)) array = new Node*[len];
	      else
		{
		  Node** p = cast(Node**)Alloc.alloc(len*(Node*).sizeof);
		  array = p[0..len];

		  // Initialize memory if we have to
		  static if(!Alloc.autoinit)
		    array[] = null;
		}
	      bitmask = len-1; // Set all bits below highBit.
	      return;
	    }
	}
      assert(0);
    }
}

// Allocator using Malloc
struct Malloc
{
  static const bool autoinit = false; // malloc does not initialize memory
  static void* alloc(uint size) { return cmalloc(size); }
  static void free(void* p) { cfree(p); }
}

// Simple but fast hash function for strings. Speed-wise on par with
// using the default hasher DefHash. Both are faster than the DMD
// built-in AAs. Use this as a base if you need to make special
// variations, such as the CITextHash hasher below.
struct SimpleTextHash
{
  static int isEqual(char[] a, char[] b)
  { return !typeid(char[]).compare(&a, &b); }

  static uint hash(char[] s)
  {
    uint hash;
    foreach (char c; s) hash = (hash * 37) + c;
    return hash;
  }
}

// Case insensitive hash function, almost as fast as the above.
struct CITextHash
{
  static const char conv = 'a'-'A';

  static int isEqual(char[] a, char[] b)
  { return !icmp(a,b); }

  static uint hash(char[] s)
  {
    uint hash;
    foreach (char c; s)
      {
	if(c <= 'Z' && c >= 'A') c += conv;
	hash = (hash * 37) + c;
      }
    return hash;
  }
}

unittest
{
  // Test some basic template instants
  alias HashTable!(int, int, Malloc) II;
  alias HashTable!(char[], int, GCAlloc, SimpleTextHash) CI;
  alias HashTable!(II,CI) COMPLEX;

  CI ci;

  int i = ci.tabSize();
  ci.rehash();
  assert(ci.tabSize == i); // A rehash should not do anything for so
			   // few values.
  ci.rehashShrink();
  assert(ci.tabSize != i); // But it does if we allow it to shring

  // Lookup before the list is created
  assert(!ci.inList("abcd", i));

  // Assign some values and test
  ci["Hei"] = 10;
  ci["Hopp,"] = -34;
  assert(ci.insert("Kall2", 5) == 5);
  assert((ci["Kall2"] = 20) == 20); // Overwrite value
  assert(ci.length == 3);

  // Test inList
  assert(ci.inList("Hei"));
  assert(ci.inList("Hopp,",i));
  assert(i == -34);
  assert(!ci.inList("hei"));

  // Test default values
  assert(ci["Kall2"] == 20);
  assert(ci["Kall2", 123] == 20);
  assert(ci["aa", 13] == 13);
  assert(ci["aa", 31] == 13);
  assert(ci["aa"] == 13);

  // Get a pointer
  int *ip;
  int *ip2;
  assert(ci.insertEdit("bb", ip) == true);
  *ip = 3;
  assert(ci.insertEdit("bb", ip2) == false);
  assert(ip == ip2);
  *ip2 = 4;
  assert(ci["bb"] == 4);

  // opApply
  assert(ci.length == 5);
  const char[][] str = ["Hei", "Hopp,", "Kall2", "aa", "bb"];
  const int[] ia = [10, -34, 20, 13, 4];
  i = 0;
  foreach(char[] key, int val; ci)
    {
      assert(key == str[i]);
      assert(val == ia[i]);
      i++;
    }

  ci.rehash(1000);
  assert(ci.tabSize > 1000);

  // Remove elements
  assert(!ci.ifRemove("arne")); // Remove something that never was there

  assert(ci.ifRemove("Hei", i)); // Remove from head
  assert(i == 10);
  assert(!ci.ifRemove("Hei"));

  assert(ci.remove("bb") == 4); // Remove from the tail
  assert(!ci.ifRemove("bb"));

  ci.remove("Kall2"); // Remove from the middle
  assert(!ci.inList("Kall2"));

  assert(ci.length == 2);
  i = 1;
  foreach(char[] key, int val; ci)
    {
      assert(key == str[i]);
      assert(val == ia[i]);
      i+=2;
    }

  // Test for exceptions
  i = 0;
  try ci["hei"];
  catch(HashTableException e)
    {
      i = -1;
    }
  assert(i == -1);

  try ci.remove("bb");
  catch(HashTableException e)
    {
      i = -5;
    }
  assert(i == -5);
  ci.validate();

  // Insert more elements than the array length, to force collisions
  char[] s = (cast(char*)&i)[0..4];
  for(i = 1; i<ci.tabSize(); i++)
    {
      ci[s.dup] = i;
    }
  assert(ci.length == ci.tabSize + 1);
  ci.validate();

  // Test validation
  HashTable!(char[], int) valTest;
  char[] gummi = "gummi".dup;
  valTest[gummi] = 10;
  valTest.validate();
  gummi[4] = 'j';

  i = 0;
  try valTest.validate();
  catch(HashTableException e)
    {
      i = 3;
    }
  assert(i==3);

  // Test the case insensitive string hasher
  HashTable!(char[], char[], GCAlloc, CITextHash) istr;
  istr["jalla"] = "Afro";
  istr["AaBbz/8"] = "Passport";
  istr["æøå"] = "Duck";

  const char[][] aa = ["jalla", "AaBbz/8", "æøå"];
  const char[][] b = ["Afro", "Passport", "Duck"];
  i = 0;
  foreach(char[] ind, char[] s; istr)
    {
      assert(ind == aa[i]);
      assert(s == b[i]);
      i++;
    }

  assert(istr["jalla"] == "Afro");
  assert(istr["JaLLa"] == "Afro");
  assert(istr["æøå", "Alien"] == "Duck");
  assert(istr["AaBbz/8", "Alien"] == "Passport");
  assert(istr["aAbBZ/8", "Alien"] == "Passport");
  istr["aabbz/8"] = "Bird flu";
  i = 0;
  foreach(char[] ind, char[] s; istr)
    {
      assert(ind == aa[i]);
      if(i != 1) assert(s == b[i]);
      else assert(s == "Bird flu");
      i++;
    }

  // Test a signless integer hash.

  struct IgnoreSign
  {
    static uint hash(int i) { return i<0 ? -i : i; }
    static int isEqual(int i, int j) { return (i==j) || (i==-j); }
  }

  HashTable!(int, char[], GCAlloc, IgnoreSign) iss;

  iss[-1] = "Afro";
  iss[2] = "Passport";
  iss[4] = "Duck";
  const int[] a = [-1, 2, 4];
  i = 0;
  foreach(int ind, char[] s; iss)
    {
      assert(ind == a[i]);
      assert(s == b[i]);
      i++;
    }

  assert(iss[-2] == "Passport");
  assert(iss[2] == "Passport");
  assert(iss[-4, "Alien"] == "Duck");
  assert(iss[4, "Alien"] == "Duck");
  iss[1] = "Bird flu";
  i = 0;
  foreach(int ind, char[] s; iss)
    {
      assert(ind == a[i]);
      if(i) assert(s == b[i]);
      else assert(s == "Bird flu");
      i++;
    }
}
