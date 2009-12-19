/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (nif.d) is part of the OpenMW package.

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

module nif.nif;

import nif.base;
import nif.niffile;
import nif.record;

import core.memory;

/* The NIFMesh struct will open, read and close a .nif file. Unlike
 * NIFFile (which it uses), it stores all the data from the NIF.
 */
NIFMesh nifMesh;

struct NIFMesh
{
 public:
  Record records[];

  // Read from a file
  void open(char[] file)
  {
    nifFile.open(file);
    loadData();		// Load the data 
  }

  // Read from a memory slice. The optitional file name is used for
  // error messages only.
  void open(void[] s, char[] name = "")
  {
    nifFile.open(s, name);
    loadData();		// Load the data 
  }

  // Clear all loaded data
  void clear()
    {
      records = null;

      // Clear the region manager
      nifRegion.freeAll();
    }

  /*
  void fail(char[] msg)
    {
      throw new NIFMeshException(msg);
    }
  */

  private void loadData()
    {
      // Remove any previously loaded data
      clear();

      // The actual parsing is done somewhere else.
      nif.base.parseFile();

      // Close the file
      nifFile.close();

      // Resolve inter-record references into object pointers.
      foreach(int i, Record o; records)
	if(o !is null)
	  {
	    nifFile.setRec(i, o);
	    o.sortOut(records);
	  }

      // Let the records do consistency checks on their data.
      foreach(int i, Record o; records)
	if(o !is null)
	  {
	    nifFile.setRec(i, o);
	    o.check();
	  }

      // Internal state check, used for debugging
      debug(statecheck)
	foreach(Record o; records)
	  if(o !is null) o.finalCheck();
    }

}
