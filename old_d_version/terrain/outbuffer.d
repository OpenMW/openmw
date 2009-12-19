/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (outbuffer.d) is part of the OpenMW package.

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

/*
  This files provides a simple buffer class used for writing the cache
  files. It lets you 'write' data to a growing memory buffer and
  allows you to change the written data after the fact (since it's
  retained in memory.) When you're done, you can write the entire
  buffer to a stream in one operation.
 */

module terrain.outbuffer;

import util.regions;
import std.stream;

class OutBuffer
{
 private:
  RegionManager reg;
  long used;
  void[][] buffers;

 public:
  this()
    {
      reg = new RegionManager("Outbuf", 200*1024);
    }

  void reset()
    {
      if(buffers.length)
        delete buffers;

      reg.freeAll();
      used = 0;
      buffers = null;
    }

  // Write everyting to a stream as one buffer
  void writeTo(Stream str)
    {
      foreach(void[] v; buffers)
        str.writeExact(v.ptr, v.length);

      reset();
    }

  // Get a pointer to a new block at least 'bytes' large, but don't
  // add it to the list.
  void[] reserve(size_t bytes)
    { return reg.allocate(bytes); }

  // Get a new block which is 'bytes' size large.
  void[] add(size_t bytes)
    {
      void[] p = reserve(bytes);
      add(p);
      return p;
    }

  // Add an existing block to the write list
  void add(void[] p)
  {
    buffers ~= p;
    used += p.length;
  }

  T[] write(T)(size_t num)
  {
    return cast(T[])add(num * T.sizeof);
  }

  size_t size() { return used; }
}
