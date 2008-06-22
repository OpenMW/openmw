/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bsafile.d) is part of the OpenMW package.

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
module bsa.bsafile;

//debug=checkHash;

debug(checkHash) import std.stdio;

// This file does not have any unit tests, since you really need the
// data to test it. Use the program named 'bsatool', it uses the NIF
// reader library and scans through a bsa archive, providing a good
// test of both libraries.

//import std.stream;
import std.string;
import std.mmfile;

import core.memory;

import monster.util.aa;

class BSAFileException : Exception
{
  this(char[] msg) {super("BSAFileException: " ~ msg);}
}

/**
 * This class is used to read "Bethesda Archive Files", or BSAs.
 *
 * The BSA archives are typically held open for the entire lifetime of
 * the application, and are accessed more or less randomly. For that
 * reason the BSAFile class uses memory mapped files. However, to be
 * reasonably memory efficient, only the last requested file is
 * guaranteed to be mapped at any given time, therefore make sure you
 * don't use any persistant slices.
 *
 */

class BSAFile
{
 private:
  // Size of the blocks to map with the memory mapped file. If set to
  // 0, we map the entire file, but then we use a LOT of system
  // memory. There is really no penalty in setting it too small, since
  // multiple blocks may be mapped simultaneously. However, it MUST be
  // a multiple of the system page size. TODO: On my system it is 4K,
  // later on I will have to call getpagesize and the windows
  // equivalent to find this. For now I just assume 8K is ok.
  static int pageSize = 8*1024;

  // Represents one file entry in the archive
  struct FileStruct
  {
    // File size and offset in file. We store the offset from the
    // beginning of the file, not the offset into the data buffer
    // (which is what is stored in the archive.)
    uint fileSize, offset;
    char[] name;

    void getName(char[] buf, uint start)
    {
      if(start >= buf.length)
	throw new BSAFileException("Name offset outside buffer");

      uint end = start;
      // We have to search for the end of the name string, marked by a zero.
      for(; end<buf.length; end++)	
	if(buf[end] == 0) break;

      if(end == buf.length)
	throw new BSAFileException("String buffer overflow");

      name = buf[start..end];
    }

    // This currently isn't needed, but it would be if we wanted to
    // write our own bsa archives.
    debug(checkHash)
    {
      void hashName(out uint hash1, out uint hash2)
	{
	  uint sum, off, temp, n;

	  foreach(char c; name[0..$/2])
	    {
	      sum ^= (cast(uint)c) << (off & 31);
	      off += 8;
	    }
	  hash1 = sum;

	  sum = off = 0;

	  foreach(char c; name[$/2..$])
	    {
	      temp = (cast(uint)c) << (off & 31);
	      sum ^= temp;
	      n = temp & 0x1F;
	      sum = (sum << (32-n)) | (sum >> n); // "rotate right" operation
	      off += 8;
	    }
	  hash2 = sum;
	}
    }
  }

  MmFile mmf;		// Handle to memory mapped file
  char[] filename;	// File name
  FileStruct files[];	// The file table is stored here
  bool isLoaded;	// Set to true if a file has been loaded

  // An AA for fast file name lookup. The CITextHash is a
  // case-insensitive text hasher, meaning that all lookups are case
  // insensitive.
  HashTable!(char[], int, ESMRegionAlloc, CITextHash) lookup;

  void fail(char[] msg)
    {
      throw new BSAFileException(msg ~ "\nFile: " ~ filename);
    }

  void read()
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
       * - 8 bytes*numfiles, each record contains
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
       * - 8*filenum - hast table block, we currently ignore this
       *
       * Data buffer:
       *
       * - The rest of the archive is file data, indexed by the
       *   offsets in the directory block. The offsets start at 0 at
       *   the beginning of this buffer.
       *
       */
      assert(!isLoaded);

      uint fsize = mmf.length;

      if( fsize < 12 )
	fail("File too small to be a valid BSA archive");

      // Recast the file header as a list of uints
      uint[] array = cast(uint[]) mmf[0..12];

      if(array[0] != 0x100)
	fail("Unrecognized BSA header");

      // Total number of bytes used in size/offset-table + filename
      // sections.
      uint dirsize = array[1];
      debug writefln("Directory size: ", dirsize);

      // Number of files
      uint filenum = array[2];
      debug writefln("Number of files: ", filenum);

      // Each file must take up at least 21 bytes of data in the
      // bsa. So if files*21 overflows the file size then we are
      // guaranteed that the archive is corrupt.
      if( (filenum*21 > fsize -12) ||
	  (dirsize+8*filenum > fsize -12) )
	fail("Directory information larger than entire archive");

      // Map the entire directory (skip the hashes if we don't need them)
      debug(checkHash)
	void[] mm = mmf[12..(12+dirsize+8*filenum)];
      else
	void[] mm = mmf[12..(12+dirsize)];

      // Allocate the file list from esmRegion
      files = esmRegion.allocateT!(FileStruct)(filenum);

      // Calculate the offset of the data buffer. All file offsets are
      // relative to this.
      uint fileDataOffset = 12 + dirsize + 8*filenum;

      // Get a slice of the size/offset table
      array = cast(uint[])mm[0..(8*filenum)];
      int index = 0; // Used for indexing array[]	

      // Read the size/offset table
      foreach(ref FileStruct fs; files)
	{
	  fs.fileSize = array[index++];
	  fs.offset = array[index++] + fileDataOffset;
	  if(fs.offset+fs.fileSize > fsize) fail("Archive contains files outside itself");
	}

      // Get a slice of the name offset table
      array = cast(uint[])mm[(8*filenum)..(12*filenum)];

      // Get a slice of the name field
      char[] nameBuf = cast(char[])mm[(12*filenum)..dirsize];
      // And copy it!
      nameBuf = esmRegion.copy(nameBuf);

      // Tell the lookup table how big it should be
      lookup.rehash(filenum);

      // Loop through the name offsets and pick out the names
      foreach(int idx, ref FileStruct fs; files)
	{
	  fs.getName(nameBuf, array[idx]);
	  lookup[fs.name] = idx;
	  debug(2) writefln("%d: %s, %d bytes at %x", idx,
			    fs.name, fs.fileSize, fs.offset);
	}

      // Code to check if file hashes are correct - this was mostly
      // used to check our hash algorithm.
      debug(checkHash)
	{
	  // Slice the Hash table
	  array = cast(uint[])mm[dirsize..(dirsize+8*filenum)];
	  index = 0;
	  foreach(ref FileStruct fs; files)
	    {
	      uint h1, h2;
	      fs.hashName(h1,h2);
	      uint h11 = array[index++];
	      uint h22 = array[index++];
	      if(h1 != h11) writefln("1 : %d vs. %d", h1, h11);
	      if(h2 != h22) writefln("2 : %d vs. %d", h2, h22);
	    }
	}

      isLoaded = true;
    }

 public:

  /* -----------------------------------
   * BSA management methods
   * -----------------------------------
   */

  this() {}
  this(char[] file) {open(file);}

  // We should clean up after us
  ~this() {clear();}

  // Open a new file. Clears any existing data
  void open(char[] file)
    {
      clear();
      filename = file;

      // Open a memory mapped file
      mmf = new MmFile(
		    file, 			// File name
		    MmFile.Mode.Read,		// Read only
		    0,				// We need the entire file
		    null,			// Don't care where it is mapped
		    pageSize);			// DON'T map the entire file, uses
                                                // too much memory.
      // Load header and file directory
      read();
    }

  // Clear all data and close file
  void clear()
    {
      delete mmf;

      lookup.reset;
      files.length = 0;
      isLoaded = false;
    }

  /* -----------------------------------
   * Archive file routines
   * -----------------------------------
   */

  void[] findSlice(int index)
    {
      if(!isLoaded)
	fail("No archive is open");

      if(index < 0 || index >= files.length)
	fail("Index out of bounds");

      //writefln("\noffset %d, filesize %d", files[index].offset, files[index].fileSize);
      // Get a slice of the buffer that comprises this file
      with(files[index])
	return mmf[offset..offset+fileSize];
    }

  int getIndex(char[] name)
    {
      int i;

      // Look it up in the AA
      if( lookup.inList( name, i ) )
	return i;
      else
	return -1;
    }

  // Return a slice. This routine converts the name to lower case,
  // since all BSA file names are stored that way, but references to
  // them in ESM/ESPs and NIFs are not.
  void[] findSlice(char[] name)
    {
      int i = getIndex(name);
      if(i == -1) return null;
      return findSlice(i);
    }

  // Used by the 'bsatest' program to loop through the entire
  // archive.
  FileStruct[] getFiles() { return files; }

  // Number of files in the archive
  uint numFiles() { return files.length; }

  // Gets the name of the archive file.
  char[] getName() { return filename; }
}
