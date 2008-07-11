/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (random.d) is part of the OpenMW package.

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

module util.random;

private import std.date;
private import std.random;

abstract class Random
{
  static const double scale = 1.0/uint.max;

  // Initialize from current time
  this() { initialize(); }

  // Initialize from parameter
  this(long seed) { initialize(seed); }

  // Reinitialize with seed 
  abstract void initialize(long newSeed);

  // Produce random numbers between 0 and uint.max
  abstract uint rand();

  // Default is to initialize using current time as seed
  void initialize() { initialize(getUTCtime()); }

  // Produce a uniform random number between 0 and 1
  double random()
    {
      return rand() * scale;
    }

  // Return a uniform random number between a and b. Works for the
  // both the cases a < b and a > b.
  double random(double a, double b)
    in
    {
      // We only disallow nan parameters
      assert(a <>= b);
    }
    out(result)
    {
      // Result must be in range m < result < M, where m=min(a,b) and M=max(a,b)
      if(b > a) assert( (a < result) && (result < b) );
      else if(a > b) assert( (b < result) && (result < a) );
    }
    body
    { return random()*(b - a) + a; }

  // Return a random integer between a and b, inclusive.
  int randInt(int a, int b)
    out(result)
    {
      // Result must be in range m <= result <= M, where m=min(a,b) and M=max(a,b)
      if(b >= a) assert( (a <= result) && (result <= b) );
      else if(a > b) assert( (b <= result) && (result <= a) );
    }
    body
    {
      if(a>b) return cast(int)(rand() % (a-b+1)) + b;
      else if(b>a) return cast(int)(rand() % (b-a+1)) + a;
      else return a;
    }

  // Allow using "function call" syntax:
  //
  //   Random ran = new Random1;
  //   double d = ran();		// Calls ran.random()
  //   d = ran(a,b);			// Calls ran.random(a,b);
  double opCall() { return random(); }
  double opCall(double a, double b) { return random(a, b); }

  // Return the seed originally given the object
  long getSeed() {return origSeed;}

protected:
  long origSeed; // Value used to seed the generator
}

// Uses the standard library generator
class DRand : Random
{
  // Initialize from current time
  this() { super(); }

  // Initialize from parameter
  this(long seed) { super(seed); }

  uint rand() { return std.random.rand(); }

  void initialize(long newSeed)
    {
      origSeed = newSeed;
      rand_seed(cast(uint)newSeed, 0);
    }

  alias Random.initialize initialize;

  unittest
  {
    struct tmp { import std.stdio; }
    alias tmp.writef writef;
    alias tmp.writefln writefln;

    writefln("Unittest for class DRand");

    DRand ran = new DRand;
    writefln("Seed (from time) = ", ran.getSeed());

    // Take a look at some numbers on screen
    writefln("Some random numbers in [0,1]:");
    for(int i=0; i<10; i++)
      writef("  ", ran());

    ran = new DRand(0);
    writefln("\nNew seed (preset) = ", ran.getSeed());

    // Take a look at some numbers on screen
    writefln("Some random numbers in [0,1]:");
    for(int i=0; i<10; i++)
      writef("  ", ran());

    // Check that all interfaces work (compile time)
    ran();
    ran(1,2);
    ran.random();
    ran.random(3,4);
    ran.initialize();
    ran.initialize(10);
    ran.randInt(-3,5);

    writefln("\nEnd of unittest for class DRand\n");
  }
}
