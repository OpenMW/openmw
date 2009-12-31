/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (bsa_file.h) is part of the OpenMW package.

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

#ifndef _BSA_FILE_H_
#define _BSA_FILE_H_

#include "mangle/stream/servers/file_stream.h"
#include "mangle/stream/filters/slice_stream.h"

#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <vector>
#include <map>

#include "../tools/str_exception.h"

/**
   This class is used to read "Bethesda Archive Files", or BSAs.
 */
class BSAFile
{
 private:

  /// Represents one file entry in the archive
  struct FileStruct
  {
    // File size and offset in file. We store the offset from the
    // beginning of the file, not the offset into the data buffer
    // (which is what is stored in the archive.)
    uint32_t fileSize, offset;

    // Zero-terminated file name
    char* name;
  };

  /// The archive source
  Mangle::Stream::Stream *input;

  /// Table of files in this archive
  std::vector<FileStruct> files;

  /// Filename string buffer
  std::vector<char> stringBuf;

  /// True when an archive has been loaded
  bool isLoaded;

  /// Used for error messages
  std::string filename;

  /// Non-case sensitive string comparison
  struct iltstr
  {
    bool operator()(const char *s1, const char *s2) const
    { return strcasecmp(s1,s2) < 0; }
  };

  /** A map used for fast file name lookup. The value is the index into
      the files[] vector above. The iltstr ensures that file name
      checks are non-case sensitive.
  */
  typedef std::map<const char*, int, iltstr> Lookup;
  Lookup lookup;

  /// Error handling
  void fail(const std::string &msg)
    {
      throw str_exception("BSA Error: " + msg + "\nArchive: " + filename);
    }

  /// Read header information from the input source
  void readHeader()
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

    // Each file must take up at least 21 bytes of data in the
    // bsa. So if files*21 overflows the file size then we are
    // guaranteed that the archive is corrupt.
    if( (filenum*21 > fsize -12) ||
        (dirsize+8*filenum > fsize -12) )
      fail("Directory information larger than entire archive");

    // Read the offset info into a temporary buffer
    std::vector<uint32_t> offsets;
    offsets.resize(3*filenum);
    input->read(&offsets[0], 12*filenum);

    // Read the string table
    stringBuf.resize(dirsize-12*filenum);
    input->read(&stringBuf[0], stringBuf.size());

    // Check our position
    assert(size->tell() == 12+dirsize);

    // Calculate the offset of the data buffer. All file offsets are
    // relative to this. 12 header bytes + directory + hash table
    // (skipped)
    size_t fileDataOffset = 12 + dirsize + 8*filenum;

    // Set up the the FileStruct table
    files.resize(filenum);
    for(int i=0;i<filenum;i++)
      {
        FileStruct &fs = files[i];
        fs.fileSize = offsets[i*2];
        fs.offset = offsets[i*2+1] + fileDataOffset;
        fs.name = &stringBuf[offsets[2*numfiles+i]];

        if(fs.offset + fs.fileSize > fsize)
          fail("Archive contains offsets outside itself");

        // Add the file name to the lookup
        lookup[fs.name] = i;
      }

    isLoaded = true;
  }

  /// Get the index of a given file name, or -1 if not found
  int getIndex(const char *str)
  {
    Lookup::iterator it;
    it = lookup.find(str);

    if(it == lookup.end()) return -1;
    else
      {
        int res = it->second;
        assert(res >= 0 && res < files.size());
        return res;
      }
  }

 public:

  /* -----------------------------------
   * BSA management methods
   * -----------------------------------
   */

  BSAFile()
    : input(NULL), isLoaded(false) {}

  /// Open an archive file.
  void open(const std::string &file)
  {
    filename = file;
    input = new Mangle::Stream::FileStream(file);
    read();
  }

  /** Open an archive from a generic stream. The 'name' parameter is
      used for error messages.
  */
  void open(Mangle::Stream::Stream* inp, const std::string &name)
  {
    filename = name;
    input = inp;
    read();
  }

  /* -----------------------------------
   * Archive file routines
   * -----------------------------------
   */

  /// Check if a file exists
  bool exists(const char *file)
  {
    return getIndex(file) != -1;
  }

  /** Open a file contained in the archive. Throws an exception if the
      file doesn't exist.

      NOTE: All files opened from one archive will share a common file
      handle. This is NOT thread safe.
   */
  Mangle::Stream::Stream *getFile(const char *file)
  {
    int i = getIndex(file);
    if(i == -1)
      fail("File not found: " + std::string(file));

    FileStruct &fs = files[i];

    return new SliceStream(input, fs.offset, fs.fileSize);
  }
};

#endif
