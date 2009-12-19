/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (misc.d) is part of the OpenMW package.

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

// This doesn't have to be part of the nif package at all.
module nif.misc;

import std.string;
import monster.util.string;

struct Vector
{
  float array[3];

  void set(float x, float y, float z)
  {
    array[0] = x;
    array[1] = y;
    array[2] = z;
  }

  char[] toString()
  {
    return format(array);
    //return format("[", array[0], ",", array[1], ",", array[2], "]");
  }

  int opEquals(ref Vector v)
  {
    return v.array == array;
  }

  static assert(Vector.sizeof == 4*3);
}

unittest
{
  Vector a, b;
  a.set(1,2,3);
  assert(a!=b);
  b = a;
  assert(a==b);
}

struct Vector4
{
  float array[4];

  void set(float x, float y, float z, float a)
  {
    array[0] = x;
    array[1] = y;
    array[2] = z;
    array[3] = a;
  }

  char[] toString()
  {
    return format(array);
    //return format("[", array[0], ",", array[1], ",", array[2], ",", array[3], "]");
  }

  int opEquals(ref Vector4 v)
  {
    return v.array == array;
  }

  static assert(Vector4.sizeof == 4*4);
}

unittest
{
  Vector4 a, b;
  a.set(1,2,3,5);
  assert(a!=b);
  b = a;
  assert(a==b);
}

align(1)
struct Matrix
{
  union
  {
    Vector v[3];
    float[9] array;
  }

  char[] toString()
  {
    char[] res;
    res ~= "  Right: " ~ v[0].toString;
    res ~= "\n  Up:    " ~ v[1].toString;
    res ~= "\n  Front: " ~ v[2].toString;
    return res;
  }
  static assert(Matrix.sizeof == 3*3*4);
}


align(1)
struct Transformation
{
  Vector pos;
  Matrix rotation;
  float scale;
  Vector velocity;

  char[] toString()
  {
    char[] res;
    res ~= "  Translation: " ~ pos.toString();
    res ~= "\n" ~ rotation.toString();
    res ~= "\n  Scale: " ~ format(scale);
    res ~= "\n  Velocity: " ~ velocity.toString();
    return res;
  }

  static assert(Transformation.sizeof == 5*Vector.sizeof + 4);
}
