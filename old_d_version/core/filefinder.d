/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (filefinder.d) is part of the OpenMW package.

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

module core.filefinder;

import std.file;
import std.string;

import monster.util.string;
import monster.util.aa;

import core.memory;

import std.stdio;

class FileFinderException : Exception
{
  this(char[] msg, char[] ext, char[] dir)
    {
      if(ext.length) super(format("FileFinder for %s files in %s: %s", ext, dir, msg));
      else super(format("FileFinder for %s: %s", dir, msg));
    }
}

// Do we traverse directories recursively? Default is yes.
enum Recurse { Yes, No }

// The file finder is used to list all files in a directory so we can
// look up files without searching the filesystem each time. It is
// case insensitive on all platforms, and transparently converts to
// the right directory separator character (\ or /). We might extend
// it later with code from other projects.
class FileFinder
{
  private:
  char[][] files; // Use GC for this, it's not too big and we don't
		  // have to manage roots pointing to the filenames.
  HashTable!(char[], int, ESMRegionAlloc, FilenameHasher) lookup;

  char[] dir; // Base directory to search
  char[] ext; // Extensions to pick out

  void fail(char[] err)
    {
      throw new FileFinderException(err, ext, dir);
    }

  // Removes the part of a path that is stored in 'dir'
  char[] removeDir(char[] path)
    {
      //TODO: Should this be case insensitive?
      assert(path[0..dir.length] == dir);

      return path[dir.length..$];
    }

  void insert(char[] filename)
    {
      // Only keep the part of the filename not given in 'dir'.
      char[] name = removeDir(filename);

      if(!name.iEnds(ext)) return;

      // We start counting from 1
      uint newVal = files.length+1;

      // Insert it, or get the old value if it already exists
      uint oldVal = lookup[name, newVal];
      if(oldVal != newVal)
	fail("Already have " ~ name ~ "\nPreviously inserted as " ~ files[oldVal-1]);

      // Store it
      files ~= filename;
    }

 public:

  static char[] addSlash(char[] dir)
    {
      // Add a trailing slash
      version(Windows) if(!dir.ends("\\")) dir ~= '\\';
      version(Posix) if(!dir.ends("/")) dir ~= '/';
      return dir;
    }

  int length() { return lookup.length; }

  this(char[] dir, char[] ext = null, Recurse r = Recurse.Yes)
    in
    { 
      if(!dir.length) fail("'dir' can not be empty");
    }
    out
    {
      assert(files.length == lookup.length);
    }
    body
    {
      // Add a trailing slash
      dir = addSlash(dir);

      this.dir = dir;

      if(ext.length && ext[0] != '.') ext = "." ~ ext;
      this.ext = ext;

      bool callback(DirEntry* de)
	{
	  if (de.isdir)
	    {
	      if(r == Recurse.Yes)
		listdir(de.name, &callback);
	    }
	  else
	    insert(de.name);
	  return true;
	}

      try listdir(dir, &callback);
      catch(FileException e)
	fail(e.toString);
    }

  char[] opIndex(int i) { return files[i-1]; }

  int opIndex(char[] file)
    {
      int i;

      // Get value if it exists
      if(lookup.inList(file, i))
	return i;
      return 0;
    }

  bool has(char[] file)
    {
      return lookup.inList(file);
    }

  int opApply(int delegate(ref char[]) del)
    {
      int res = 0;

      foreach(char[] s; files)
	{
	  char[] tmp = removeDir(s);
	  res = del(tmp);
	  if(res) break;
	}
      return res;
    }

  char[] toString()
    {
      char[] result;
      foreach(char[] s; this)
	result ~= s ~ "\n";
      return result;
    }
}

// Hash functions that does not differentiate between linux and
// windows file names. This means that it is case insensitive, and
// treats '\' and '/' as the same character. Only needed in linux, in
// windows just use CITextHasher.
version(Posix)
struct FilenameHasher
{
  static const char conv = 'a'-'A';

  static int isEqual(char[] aa, char[] bb)
  {
    if(aa.length != bb.length) return 0;

    foreach(int i, char a; aa)
      {
	char b = bb[i];

	if(a == b)
	  continue;

	// Convert both to lowercase and "/ case"
	if(a <= 'Z' && a >= 'A') a += conv;
	else if(a == '\\') a = '/';
	if(b <= 'Z' && b >= 'A') b += conv;
	else if(b == '\\') b = '/';

	if(a != b) return 0;
      }

    // No differences were found
    return 1;
  }

  static uint hash(char[] s)
  {
    uint hash;
    foreach (char c; s)
      {
	if(c <= 'Z' && c >= 'A') c += conv;
	else if(c == '\\') c = '/';
	hash = (hash * 37) + c;
      }
    return hash;
  }
}

version(Windows) alias CITextHash FilenameHasher;
