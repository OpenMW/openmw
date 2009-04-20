/*
  Monster - an advanced game scripting language
  Copyright (C) 2007-2009  Nicolay Korslund
  Email: <korslund@gmail.com>
  WWW: http://monster.snaptoad.com/

  This file (vfs.d) is part of the Monster script language package.

  Monster is distributed as free software: you can redistribute it
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

module monster.modules.vfs;

import std.file;
import std.stream;
import std.string;
import monster.util.string;
import monster.vm.error;

abstract class VFS
{
  // Abstract functions. These must be implemented in child classes.

  // Return true if a file exists. Should not return true for
  // directories.
  abstract bool has(char[] file);

  // Open the given file and return it as a stream.
  abstract Stream open(char[] file);

  static final char[] getBaseName(char[] fullname)
    {
      foreach_reverse(i, c; fullname)
        {
          version(Win32)
            {
              if(c == ':' || c == '\\' || c == '/')
                return fullname[i+1..$];
            }
          version(Posix)
            {
              if (fullname[i] == '/')
                return fullname[i+1..$];
            }
        }
      return fullname;
    }
}

// A VFS that contains a list of other VFS objects
class ListVFS : VFS
{
 private:
  VFS[] list;

 public:
  this(VFS v[] ...)
    { list = v; }

  void add(VFS v[] ...)
    { list ~= v; }

  void addFirst(VFS v[] ...)
    { list = v ~ list; }

  bool has(char[] file)
    {
      foreach(l; list)
        if(l.has(file)) return true;
      return false;
    }

  Stream open(char[] file)
    {
      foreach(l; list)
        if(l.has(file)) return l.open(file);
      fail("No member VFS contains file " ~ file);
    }
}

// A VFS that reads files from a given path in the OS file
// system. Disallows filenames that escape the given path,
// ie. filenames such as:
//
// /etc/passwd
// dir/../../file
// c:\somefile
class FileVFS : VFS
{
 private:
  char[] sysPath;
  char[] buffer;

  char[] getPath(char[] file)
    {
      // Make sure the buffer is large enough
      if(buffer.length < file.length+sysPath.length)
        buffer.length = file.length + sysPath.length + 50;

      // Check for invalid file names. This makes sure the caller
      // cannot read files outside the designated subdirectory.
      if(file.begins([from]) || file.begins([to]))
        fail("Filename " ~ file ~ " cannot begin with a path separator");
      if(file.find(":") != -1)
        fail("Filename " ~ file ~ " cannot contain colons");
      if(file.find("..") != -1)
        fail("Filename " ~ file ~ " cannot contain '..'");

      // Copy the file name over
      buffer[sysPath.length .. sysPath.length+file.length]
        = file[];

      // Convert the path characters
      convPath();

      // Return the result
      return buffer[0..sysPath.length+file.length];
    }

  // Convert path separators
  void convPath()
    {
      foreach(ref c; buffer)
        if(c == from)
          c = to;
    }

  version(Windows)
    {
      const char from = '/';
      const char to = '\\';
    }
  else
    {
      const char from = '\\';
      const char to = '/';
    }

 public:
  this(char[] path = "")
    {
      // Set up the initial buffer
      buffer.length = path.length + 50;

      if(path.length)
        {
          // Slice the beginning of it and copy the path over
          sysPath = buffer[0..path.length];
          sysPath[] = path[];
        }

      convPath();

      // Make sure the last char in the path is a path separator
      if(!path.ends([to]))
        {
          sysPath = buffer[0..path.length+1];
          sysPath[$-1] = to;
        }
    }

  bool has(char[] file)
    { return exists(getPath(file)) != 0; }

  Stream open(char[] file)
    { return new BufferedFile(getPath(file)); }
}
