/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (bsa_file.cpp) is part of the OpenMW package.

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

#include "bsa_file.hpp"
#include <components/file_finder/file_finder.hpp>

#include <libs/mangle/stream/servers/file_stream.hpp>
#include <libs/mangle/stream/filters/slice_stream.hpp>

#include <stdexcept>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

using namespace std;
using namespace Mangle::Stream;

/// Error handling
void BSAFile::fail(const string &msg)
{
  throw std::runtime_error("BSA Error: " + msg + "\nArchive: " + filename);
}

/// Read header information from the input source
void BSAFile::readHeader()
{
  /*
   * The layout of a BSA archive is as follows:
   *
   * - 12 bytes header, contains 3 ints:
   *         id number - equal to 0x100
   *         dirsize - size of the directory block (see below)
   *         numfiles - number of files
   *
   * ---------- start of directory block -----------
   *
   * - 8 bytes*numfiles, each record contains:
   *         fileSize
   *         offset into data buffer (see below)
   *
   * - 4 bytes*numfiles, each record is an offset into the following name buffer
   *
   * - name buffer, indexed by the previous table, each string is
   *   null-terminated. Size is (dirsize - 12*numfiles).
   *
   * ---------- end of directory block -------------
   *
   * - 8*filenum - hash table block, we currently ignore this
   *
   * ----------- start of data buffer --------------
   *
   * - The rest of the archive is file data, indexed by the
   *   offsets in the directory block. The offsets start at 0 at
   *   the beginning of this buffer.
   *
   */
  assert(!isLoaded);
  assert(input);
  assert(input->hasSize);
  assert(input->hasPosition);
  assert(input->isSeekable);

  // Total archive size
  size_t fsize = input->size();

  if( fsize < 12 )
    fail("File too small to be a valid BSA archive");

  // Get essential header numbers
  size_t dirsize, filenum;

  {
    // First 12 bytes
    uint32_t head[3];

    input->read(head, 12);

    if(head[0] != 0x100)
      fail("Unrecognized BSA header");

    // Total number of bytes used in size/offset-table + filename
    // sections.
    dirsize = head[1];

    // Number of files
    filenum = head[2];
  }

  // Each file must take up at least 21 bytes of data in the bsa. So
  // if files*21 overflows the file size then we are guaranteed that
  // the archive is corrupt.
  if( (filenum*21 > fsize -12) ||
      (dirsize+8*filenum > fsize -12) )
    fail("Directory information larger than entire archive");

  // Read the offset info into a temporary buffer
  vector<uint32_t> offsets(3*filenum);
  input->read(&offsets[0], 12*filenum);

  // Read the string table
  stringBuf.resize(dirsize-12*filenum);
  input->read(&stringBuf[0], stringBuf.size());

  // Check our position
  assert(input->tell() == 12+dirsize);

  // Calculate the offset of the data buffer. All file offsets are
  // relative to this. 12 header bytes + directory + hash table
  // (skipped)
  size_t fileDataOffset = 12 + dirsize + 8*filenum;

  // Setup FileFinder

  FileFinder::FileFinder data_files(data_dir);

  // Set up the the FileStruct table
  files.resize(filenum);
  for(size_t i=0;i<filenum;i++)
    {
      FileStruct &fs = files[i];
      fs.fileSize = offsets[i*2];
      fs.offset = offsets[i*2+1] + fileDataOffset;
      fs.name = &stringBuf[offsets[2*filenum+i]];

      if(fs.offset + fs.fileSize > fsize)
        fail("Archive contains offsets outside itself");



          if(!data_files.has(fs.name)) {
          // Add the file name to the lookup
            lookup[fs.name] = i;
            fs.external = false;
          }
          else {
              fs.external= true;
              externals.push_back(StreamPtr(new FileStream(data_files.lookup(fs.name))));

              int externals_i = (externals.size() - 1);

              assert(externals[externals_i]);
              assert(externals[externals_i]->hasPosition);
              assert(externals[externals_i]->isSeekable);

              fs.offset = externals_i;
              fs.fileSize = externals[fs.offset]->size();

              lookup[fs.name] = i;
         }
    }

  isLoaded = true;
}

/// Get the index of a given file name, or -1 if not found
int BSAFile::getIndex(const char *str)
{
  Lookup::iterator it;
  it = lookup.find(str);

  if(it == lookup.end()) return -1;
  else
    {
      int res = it->second;
      assert(res >= 0 && res < static_cast<int> (files.size()));
      return res;
    }
}

/// Open an archive file.
void BSAFile::open(const string &file, const string &data)
{
  filename = file;
  data_dir = data;
  input = StreamPtr(new FileStream(file));
  readHeader();
}

/** Open an archive from a generic stream. The 'name' parameter is
    used for error messages.
*/
void BSAFile::open(StreamPtr inp, const string &name, const string &data)
{
  filename = name;
  input = inp;
  readHeader();
}

StreamPtr BSAFile::getFile(const char *file)
{
  assert(file);
  int i = getIndex(file);
  if(i == -1)
    fail("File not found: " + string(file));

  FileStruct &fs = files[i];

  if(!fs.external)
    return StreamPtr(new SliceStream(input, fs.offset, fs.fileSize));
  else
    return StreamPtr(new SliceStream(externals[fs.offset], 0, fs.fileSize));
}
