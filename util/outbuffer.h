/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (outbuffer.h) is part of the OpenMW package.

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

// This is sort of like a mini-version of the Region class in
// D. FIXME: And it doesn't need to be. Rewrite this to add buffers of
// the exact size requested instead of filling a buffer of predefined
// size.
class OutBuffer
{
 public:
  OutBuffer() : used(0), left(0), buffers(), sizes()
    {}

  ~OutBuffer()
    {
      deallocate();
    }

  // Write everyting to a stream as one buffer
  void writeTo(std::ostream &str)
    {
      for(int i=0;i<buffers.size();i++)
        str.write((char*)buffers[i], sizes[i]);
    }

  // Get a pointer to a new block at least 'bytes' large. Allocate a
  // new buffer if necessary.
  void *reserve(size_t bytes)
    {
      assert(bytes <= bufSize);

      if(left >= bytes)
        return curPtr;

      // Not enough space left. Allocate a new buffer.
      curPtr = (char*)malloc(bufSize);
      left = bufSize;

      // Store the new buffer in the lists
      buffers.push_back(curPtr);
      sizes.push_back(0);

      return curPtr;
    }

  // Get a new block which is 'bytes' size large. The block will be
  // marked as 'used'.
  void *add(size_t bytes)
    {
      void *res = reserve(bytes);

      if(bytes == 0)
        return res;

      assert(left >= bytes);
      curPtr += bytes;
      left -= bytes;

      // We keep a count of the total number of bytes used
      used += bytes;

      // Keep a count for each buffer as well
      sizes[sizes.size()-1] += bytes;

      return res;
    }

  template <class T>
  T* write(size_t num)
  {
    return (T*)add(num*sizeof(T));
  }

  void deallocate()
    {
      for(int i=0;i<buffers.size();i++)
        free(buffers[i]);

      buffers.clear();
      sizes.clear();

      left = 0;
      used = 0;
    }

  size_t size() { return used; }

 private:
  std::vector<void*> buffers;
  std::vector<int> sizes;
  size_t used, left;
  char *curPtr;

  static const size_t bufSize = 200*1024;
};

std::ostream& operator<<(std::ostream& os, OutBuffer& buf)
{
  buf.writeTo(os);
  return os;
}

