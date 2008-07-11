/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (niffile.d) is part of the OpenMW package.

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

module nif.niffile;

public import std.stream;
public import util.regions;

import std.string;
import nif.misc;

public import core.memory;

// Exception class thrown by most exceptions originating within the
// nif package
class NIFFileException : Exception
{
  this(char[] msg) {super("NIFFile Exception: " ~ msg);}
}

/* The NIFFile struct is used for the task of reading from NIF
   files. It provides specialized methods for handling common types,
   records, etc., and also provides mechanisms for output and error
   handling. It does not store any (non-transient) NIF data.
 */
NIFFile nifFile;

struct NIFFile
{
 private:
  Stream f; // Input stream
  BufferedFile bf; // Used and reused for direct file input.
  TArrayStream!(ubyte[]) ss; // For stream reading

  // These are used for warnings and error messages only
  char[] filename;
  int recNum;		// Record number
  char[] recName;	// Record name
  Object recObj;	// Record object
 public:

  /* ------------------------------------------------
   * Object initialization and file management
   * ------------------------------------------------
   */

  // Open a file from the file system
  void open(char[] file)
    {
      close();

      // Create a BufferedFile, and reuse it later.
      if(bf is null)
	bf = new BufferedFile();

      bf.open(file);

      f = bf;
      filename = file;
    }

  // Open a memory slice, the name is used for error messages.
  void open(void[] data, char[] name)
    {
      close();

      // Create a TArrayStream, and reuse it
      if(ss is null)
	{
	  ss = new TArrayStream!(ubyte[])(cast(ubyte[])data);
	  ss.writeable = false; // Read-only
	}
      else
	{
	  // TArrayStream lacks the open() method. Instead, all the
	  // members are public.
	  ss.buf = cast(ubyte[])data;
	  ss.len = ss.buf.length;
	  ss.cur = 0;
	}

      f = ss;
      filename = name;
    }

  // Close the file handle
  void close()
    {
      // Close the buffered file
      if(f !is null && f is bf) f.close();
      f = null;

      // Zero out variables
      filename = null;
      recNum = 0;
      recName = null;
      recObj = null;
    }

  ulong size()
  in
  {
    if(f is null) fail("Cannot get size, file is not open");
  }
  body
  {
    return f.size();
  }

  bool eof() { return f.eof(); }

  void seekCur(long skip)
  in
  {
    if(f is null) fail("Cannot seek, file is not open");
  }
  body
  {
    f.seekCur(skip);
  }

  ulong position()
  {
    return f.position();
  }

  /* ------------------------------------------------
   * Error reporting
   * ------------------------------------------------
   */

  // Used to format error messages
  private char[] makeError(char[] str)
  {
    if(recNum > 0)
      {
	if(recName == "" && recObj !is null) recName = recObj.toString;
	str = format("%s\n  Record %d: %s", str, recNum-1, recName);
      }
    if(filename != "") str = format("%s\n  File: %s", str, filename);
    if(f !is null)
      if(f.eof()) str = format("%s\n  At end of file", str);
      else str = format("%s\n  Offset 0x%x", str, f.position);
    return str ~ "\n";
  }

  void fail(char[] msg)
  {
    throw new NIFFileException(makeError(msg));
  }

  debug(warnstd) import std.stdio;

  void warn(char[] msg)
  {
    debug(strict) fail("(fail on warning) " ~ msg);
      else
	{
	  debug(warnstd) writefln("WARNING: ", makeError(msg));
	  debug(warnlog) log.writefln("WARNING: ", makeError(msg));
	}
    }

  void assertWarn(bool condition, char[] str)
    { if(!condition) warn(str); }

  /*
  void assertFail(bool condition, char[] str)
    { if(!condition) fail(str); }
  */

  // Used for error message handling, hackish.
  void setRec(int i, char[] n)
    {
      recNum = i+1;
      recName = n;
      recObj = null;
    }

  // This variant takes an object instead of a name. That way we only
  // need to call toString when the name is needed (which it mostly
  // isn't.)
  void setRec(int i, Object o)
    {
      recNum = i+1;
      recObj = o;
      recName = null;
    }

  /* ------------------------------------------------
   * Reader method templates (skip to next section
   *   for usable methods)
   * ------------------------------------------------
   */

  // Read a variable of a given type T
  T getType(T)()
  {
    T t;
    f.read(t);
    return t;
  }

  // Read a variable and compare it to what we want it to be
  template getTypeIs(T)
    {
      T getTypeIs(T[] pt ...)
	{
	  T t = getType!(T)();
	  debug(check)
	    {
	      bool match;
	      foreach(T a; pt)
		if(a == t) {match = true; break;}

	      if(!match)
		{
		  char[] errMsg = format(typeid(T),
			    " mismatch: got %s, expected ", t);
		  if(pt.length > 1) errMsg ~= "one of: ";
		  foreach(T a; pt)
		    errMsg ~= format("%s ", a);
		  warn(errMsg);
		}
	    }
	  return t;
	}
    }

  debug(verbose)
    {
      // Read a variable of a given type T and print it to screen
      template wgetType(T)
	{
	  T wgetType()
	    {
	      T t;
	      f.read(t);
	      writefln(typeid(T), ": ", t);
	      return t;
	    }
	}

      // Read a variable and compare it to what we want it to be
      template wgetTypeIs(T)
	{
	  T wgetTypeIs(T pt[] ...)
	    {
	      T t = getType!(T)();

	      char[] wanted;
	      if(pt.length > 1) wanted = "one of: ";
	      foreach(T a; pt) wanted ~= format("%s ", a);

	      writefln(typeid(T), ": ", t, " (wanted %s)", wanted);
	      
	      debug(check)
		{
		  bool match;
		  foreach(T a; pt)
		    if(a == t) {match = true; break;}

		  if(!match)
		    {
		      warn(format(typeid(T),
			  " mismatch: got %s, expected %s", t, wanted));
		    }
		}
	      return t;
	    }
	}
    }

  // Fill the provided array of Ts
  template getArrayLen(T)
    {
      T[] getArrayLen(T[] arr)
	{
	  f.readExact(arr.ptr, arr.length*T.sizeof);
	  return arr;
	}
    }

  // Set the size of the provided array to 'count', and fill it.
  template getArraySize(T)
    {
      T[] getArraySize(int count)
	{
	  T[] arr;
	  fitArray(count, T.sizeof);
	  arr = cast(T[])nifRegion.allocate(count * T.sizeof);
	  getArrayLen!(T)(arr);
	  return arr;
	}
    }

  // Get an array of Ts preceded by an array length of type Index
  // (assumed to be an integer type)
  template getArray(T,Index)
    {
      T[] getArray()
	{
	  // Read array length
	  Index s = getType!(Index)();

	  // Is array larger than file?
	  fitArray(s,T.sizeof);

	  // Allocate the buffer
	  T[] result = cast(T[])nifRegion.allocate(s * T.sizeof);

	  // Read the buffer
	  return getArrayLen!(T)(result);
	}
    }

  /* ------------------------------------------------
   * Reader methods
   * ------------------------------------------------
   */

  // Strings
  alias getArrayLen!(char) getString;
  alias getArray!(char,int) getString;

  // Other arrays
  alias getArray!(int,int) getInts;

  // Checks if an array of size elements each of size esize could
  // possibly follow in the file.
  void fitArray(int size, int esize)
    in
    {
      assert(esize > 0);
    }
    body
    {
      if((size*esize+8) > (f.size() - f.position()) || size < 0)
	fail(format(
	  "Array out of bounds: %d*%d + 8 bytes needed, but only %d bytes left in file",
	  size,esize,(f.size-f.position)));
    }

  // Base types
  alias getType!(byte) getByte;
  alias getType!(ubyte) getUbyte;
  alias getType!(short) getShort;
  alias getType!(ushort) getUshort;
  alias getType!(int) getInt;
  alias getType!(uint) getUint;
  alias getType!(float) getFloat;

  // Base types with error checking
  alias getTypeIs!(short) getShortIs;
  alias getTypeIs!(ushort) getUshortIs;
  alias getTypeIs!(byte) getByteIs;
  alias getTypeIs!(ubyte) getUbyteIs;
  alias getTypeIs!(int) getIntIs;
  alias getTypeIs!(uint) getUintIs;
  alias getTypeIs!(float) getFloatIs;

  debug(verbose)
    {
      // Base types
      alias wgetType!(byte) wgetByte;
      alias wgetType!(ubyte) wgetUbyte;
      alias wgetType!(short) wgetShort;
      alias wgetType!(ushort) wgetUshort;
      alias wgetType!(int) wgetInt;
      alias wgetType!(uint) wgetUint;
      alias wgetType!(float) wgetFloat;

      // Base types with error checking
      alias wgetTypeIs!(short) wgetShortIs;
      alias wgetTypeIs!(ushort) wgetUshortIs;
      alias wgetTypeIs!(byte) wgetByteIs;
      alias wgetTypeIs!(ubyte) wgetUbyteIs;
      alias wgetTypeIs!(int) wgetIntIs;
      alias wgetTypeIs!(uint) wgetUintIs;
      alias wgetTypeIs!(float) wgetFloatIs;    
    }
  else
    {
      // Base types
      alias getByte wgetByte;
      alias getUbyte wgetUbyte;
      alias getShort wgetShort;
      alias getUshort wgetUshort;
      alias getInt wgetInt;
      alias getUint wgetUint;
      alias getFloat wgetFloat;

      // Base types with error checking
      alias getByteIs wgetByteIs;
      alias getUbyteIs wgetUbyteIs;
      alias getShortIs wgetShortIs;
      alias getUshortIs wgetUshortIs;
      alias getIntIs wgetIntIs;
      alias getUintIs wgetUintIs;
      alias getFloatIs wgetFloatIs;
    }

  // Vectors
  Vector getVector()
    {
      Vector v;
      getArrayLen!(float)(v.array);
      return v;
    }

  Vector4 getVector4()
    {
      Vector4 v;
      getArrayLen!(float)(v.array);
      return v;
    }

  Vector getVectorIs(float x, float y, float z)
    {
      Vector v = getVector();
      debug(check)
	{
	  Vector u;
	  u.set(x,y,z);
	  if(v != u)
	    warn("Vector mismatch: expected " ~ u.toString ~ ", got " ~ v.toString);
	}
      return v;
    }

  debug(verbose)
    {
      Vector wgetVector()
	{
	  Vector v = getVector();
	  writefln("vector: ", v.toString);
	  return v;
	}

      Vector4 wgetVector4()
	{
	  Vector4 v = getVector4();
	  writefln("4-vector: ", v.toString);
	  return v;
	}
    }
  else
    {
      alias getVector wgetVector;
      alias getVector4 wgetVector4;
    }

  void getMatrix(ref Matrix m)
    {
      getArrayLen!(float)(m.array);
    }

  void getTransformation(ref Transformation t)
    {
      t.pos = getVector();
      getMatrix(t.rotation);
      t.scale = getFloat/*Is(1)*/;
      t.velocity = getVectorIs(0,0,0);
    }
}
