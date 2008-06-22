/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (bsatool.d) is part of the OpenMW package.

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

module bsa.bsatool;

import std.stdio;

import bsa.bsafile;

import nif.nif;
import nif.niffile;

import core.memory;

void scanAllNifs(BSAFile f)
{
  uint totSize;
  BSAFile.FileStruct[] files = f.getFiles();

  foreach(int ind, BSAFile.FileStruct file; files)
    {
      if(file.name[$-4..$] == ".nif" ||
	 file.name[$-3..$] == ".kf" )
	{
	  void[] str = f.findSlice(ind);
	  totSize += str.length;

	  try nifMesh.open(str, file.name);
	  catch(NIFFileException e)
	    writefln("Got exception: %s", e);
	}
    }
  writefln("Total NIF/KF size in this archive: ", totSize);
}

void listFiles(BSAFile f)
{
  BSAFile.FileStruct[] files = f.getFiles();

  foreach(BSAFile.FileStruct file; files)
    writefln(file.name, " (%d bytes)", file.fileSize);
}

int main(char[][] args)
{
  //*
  int help(char[] msg)
    {
      writefln("%s", msg);
      writefln("Format: bsatool archive.bsa [-x filename] [-l] [-n]");
      writefln("  -x filename     extract file");
      writefln("  -l              list all files");
      writefln("  -n              scan through all nifs");
      return 1;
    }

  if(args.length < 3) return help("Insufficient arguments");

  initializeMemoryRegions();

  char[] filename, ext;
  bool list, nifs, extract;
  foreach(char[] a; args[1..$])
    if(a == "-l") list = true;
    else if(a == "-n") nifs = true;
    else if(a == "-x") extract = true;
    else if(extract && ext == "") ext = a;
    else if(filename == "") filename = a;
    else return help("Unknown option " ~ a);

  if(filename == "") return help("No file specified");

  auto BSAFile f = new BSAFile(filename);

  if(list) listFiles(f);
  if(nifs) scanAllNifs(f);
  if(extract)
    {
      if(ext == "") return help("No file to extract");

      void[] s = cast(ubyte[]) f.findSlice(ext);

      if(s.ptr == null)
	{
	  writefln("File '%s' not found in '%s'", ext, filename);
	  return 1;
	}
      ext = getWinBaseName(ext);

      File o = new File(ext, FileMode.OutNew);
      o.writeExact(s.ptr, s.length);
      o.close();
      writefln("File extracted to '%s'", ext);
    }

  //din.readLine();
  return 0;
}

// Picks out part of a string after the last '\'.
char[] getWinBaseName(char[] input)
{
  uint i;
  for(i = input.length; i > 0; i--)
    if(input[i-1] == '\\') break;
  return input[i..$];
}

//import std.cstream;
