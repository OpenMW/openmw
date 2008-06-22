/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (niftool.d) is part of the OpenMW package.

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

module nif.niftool;

import nif.nif;
import nif.niffile;
import std.stdio;
import std.utf;

int main(char[][] args)
{
  if(args.length < 2)
    {
      writefln("You must specify a file");
      return 1;
    }

  initializeMemoryRegions();

  bool errors;

  foreach(char[] fn; args[1..$])
    {
      writefln("Opening %s", fn);
      try nifMesh.open(fn);
      catch(NIFFileException e)
	{
	  writefln("Got exception: %s", e);
	  errors = true;
	}
      catch(UtfException e)
	{
	  writefln("Got an UTF-error: %s", e);
	  writefln("We have to ignore the rest of the file at this point. If you want to");
	  writefln("test the entire file, compile the NIF lib without verbose output.");
	}
      nifMesh.clear();
      writefln();
    }
  if(errors) return 1;
  return 0;
}
