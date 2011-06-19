/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (slice_array.h) is part of the OpenMW package.

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

#ifndef MISC_SLICE_ARRAY_H
#define MISC_SLICE_ARRAY_H

// A simple array implementation containing a pointer and a
// length. Used for holding slices into a data buffer.
#include <string.h>
#include <string>

namespace Misc
{

template <class T>
struct SliceArray
{
  const T* ptr;
  size_t length;

  /// Initialize to zero length
  SliceArray() : ptr(0), length(0) {}

  /// Initialize from pointer + length
  SliceArray(const T* _ptr, size_t _length)
    : ptr(_ptr), length(_length) {}

  /// Initialize from null-terminated string
  SliceArray(const char* str)
  {
    ptr = str;
    length = strlen(str);
  }

  bool operator==(SliceArray &t)
  {
    return
      length == t.length &&
      (memcmp(ptr,t.ptr, length*sizeof(T)) == 0);
  }

  /// Only use this for stings
  bool operator==(const char* str)
  {
    return
      str[length] == 0 &&
      (strncmp(ptr, str, length) == 0);
  }

  /** This allocates a copy of the data. Only use this for debugging
      and error messages. */
  std::string toString()
  { return std::string(ptr,length); }
};

typedef SliceArray<char> SString;
typedef SliceArray<int> IntArray;
typedef SliceArray<float> FloatArray;

}

#endif
