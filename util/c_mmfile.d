/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2009  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (c_mmfile.d) is part of the OpenMW package.

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
  This file provides a simple interface to memory mapped files
  through C functions. Since D's MmFile is superior in terms of
  usability and platform independence, we use it even for C++ code.
*/

module util.c_mmfile;

import std.mmfile;
import std.string;

version(Windows)
  static int pageSize = 64*1024;
else
  static int pageSize = 4*1024;

// List of all MMFs in existence, to keep the GC from killing them
int[MmFile] mf_list;

extern(C):

// Open a new memory mapped file
MmFile mmf_open(char *fileName)
{
  auto mmf = new MmFile(toString(fileName),
                        MmFile.Mode.Read,
                        0, null, pageSize);
  mf_list[mmf] = 1;
  return mmf;
}

// Close a file. Do not use the handle after calling this function, as
// the object gets deleted
void mmf_close(MmFile mmf)
{
  mf_list.remove(mmf);
  delete mmf;
}

// Map a region of the file. Do NOT attempt to access several regions
// at once. Map will almost always unmap the current mapping (thus
// making all current pointers invalid) when a new map is requested.
void* mmf_map(MmFile mmf, ulong offset, ulong size)
{
  return mmf[offset..offset+size].ptr;
}
