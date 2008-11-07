/*
  Monster - an advanced game scripting language
  Copyright (C) 2007, 2008  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (flags.d) is part of the Monster script language package.

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

module monster.util.flags;

// A generic bitwise flag manager.
struct Flags(T)
{
  T flags;

  void set(T t)
    { flags |= t; }

  void unset(T t)
    { flags ^= flags & t; }

  void set(T t, bool value)
    {
      if(value) set(t);
      else unset(t);
      assert(has(t) == value);
    }

  // Does it have all of the bits in the parameter set?
  bool has(T t)
    { return (flags & t) == t; }

  // Does it have any of the bits in the parameter set?
  bool hasAny(T t)
    { return (flags & t) != 0; }

  // For single-bit parameters, has() and hasAny() are identical.
}

unittest
{
  Flags!(int) fl;

  assert(fl.flags == 0);

  // Try setting some flags
  assert(!fl.has(2));
  fl.set(2);
  assert(fl.has(2));

  assert(!fl.has(4));
  fl.set(4, true);
  assert(fl.has(4));

  assert(fl.flags == 6);

  // Make sure setting them again won't change anything
  fl.set(2);
  assert(fl.flags == 6);

  fl.set(4);
  assert(fl.flags == 6);

  fl.set(2, true);
  assert(fl.flags == 6);

  fl.set(4, true);
  assert(fl.flags == 6);

  // Test the has() and hasAny() functions
  with(fl)
    {
      assert( !has(1) ); // 1 is not set
      assert( has(2) );  // 2 is set
      assert( !has(3) ); // 2+1 is NOT set, because 1 is missing
      assert( has(4) );  // 4 is set
      assert( !has(5) ); // 4+1 is NOT set, 1 is missing
      assert( has(6) );  // 2+4 is set because both are present
      assert( !has(7) ); // 1+2+4 not set, 1 missing

      assert( !hasAny(1) );// 1 is not set
      assert( hasAny(2) ); // 2 is set
      assert( hasAny(3) ); // 2 is set and part of 3=1+2
      assert( hasAny(4) ); // 4 is set
      assert( hasAny(5) ); // 4 is and is part of 5=1+4
      assert( hasAny(6) ); // 2+4 is set
      assert( hasAny(7) ); // 2+4 are part of 7
      assert( !hasAny(8) );
      assert( !hasAny(9) );
    }

  // Set a mixed flag
  fl.set(3);
  assert(fl.has(3));
  assert(fl.has(7));
  assert(fl.flags == 7);

  // Unset a flag
  fl.unset(2);
  assert(fl.flags == 5);

  // And again
  fl.unset(2);
  assert(fl.flags == 5);

  // Set and unset it with set()
  fl.set(2,true);
  assert(fl.flags == 7);
  fl.set(2,false);
  assert(fl.flags == 5);

  // Now try an enum
  enum MF
    {
      A = 1,
      B = 2,
      AB = 3,
      C = 4
    }

  Flags!(MF) mf;
  mf.set(MF.A);
  assert(mf.has(MF.A));
  assert(!mf.has(MF.AB));
  mf.set(MF.B, true);
  assert(mf.has(MF.B));
  assert(mf.has(MF.AB));
  mf.unset(MF.B);
  assert(mf.has(MF.A));
  assert(!mf.has(MF.B));
  assert(!mf.has(MF.AB));
}
