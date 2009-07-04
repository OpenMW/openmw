/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2009  Nicolay Korslund
  WWW: http://openmw.sourceforge.net/

  This file (archive.d) is part of the OpenMW package.

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

import std.stream;
import std.stdio;

// Add a couple of helper functions to the file stream
class MyFile : File
{
  this(string filename, FileMode mode = FileMode.In)
    {
      super(filename, mode);
    }

  void fill(T)(ref T t)
    {
      readExact(&t, T.sizeof);
    }

  void dump(T)(ref T t)
    {
      writeExact(&t, T.sizeof);
    }

  void fillArray(T)(T[] t)
    {
      readExact(t.ptr, t.length*T.sizeof);
    }

  void dumpArray(T)(T[] t)
    {
      writeExact(t.ptr, t.length*T.sizeof);
    }

  void readArray(T)(ref T[] arr)
    {
      int size;
      read(size);
      assert(size < 1024*1024 && size > 0);
      arr = new T[size];
      fillArray!(T)(arr);
    }

  void writeArray(T)(T[] t)
    {
      int size = t.length;
      write(size);
      dumpArray!(T)(t);
    }
}
