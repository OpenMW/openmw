/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (record.d) is part of the OpenMW package.

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

module nif.record;

public
{
  import util.regions;

  import nif.base;
  import nif.controller;
  import nif.controlled;
  import nif.node;
  import nif.data;
  import nif.extra;
  import nif.property;
  import nif.effect;

  import nif.niffile;
  import nif.misc;

  import std.string;
}

// Base class for all NIF records
abstract class Record
{
 protected:
  // List of dependency indices. We throw this away after the entire
  // file is loaded and the dependencies have been converted to object
  // pointers.
  RegionBuffer!(int) depList;

  debug(statecheck)
    {
      // An internal 'state', this check is only intended for
      // debugging purposes (hence the surrounding debug block ;-)
      int state;
                 /* 0 - nothing has been done
		  * 1 - record has been read
		  * 2 - sortOut has been called
		  * 3 - check has been called
		  */
    }
 public:

  new(uint sz)
    {
      // After eliminating all GC dependence, this is no longer
      // needed.
      //return nifRegion.allocateGC(sz).ptr;

      return nifRegion.allocate(sz).ptr;
    }

  delete(void *p) { assert(0); }

  debug(statecheck)
    final void finalCheck()
    {
      debug(veryverbose)
	writefln("Final check on ", this);
      assert(state==3);
    }

  // Read record data from file f. All indices are stored temporarily
  // as integers, as they appear in the file.
  void read()
    {
      // Allocate the dependency list. 50 should be enough entries.
      depList = nifRegion.getBuffer!(int)(0,50);

      debug(veryverbose)
	writefln("Reading ", this, " at offset %x", nifFile.position);
      debug(statecheck) assert(state++ == 0);
    }

  // Sort out dependencies between records. Called after all records
  // have been read from file. Paramter 'list' contains all records in
  // the order they appear in the file. Used to convert integer
  // indices into object pointers, and checking that all types are
  // correct. Can also be used for data checks than only require this
  // objects dependencies.
  void sortOut(Record[] list)
    {
      debug(veryverbose) writefln("Sorting out ", this);
      debug(statecheck) assert(state++ == 1);
    }

  // Consistancy check. Called after all dependencies have been
  // sorted. Can be used for checking that vertex counts are the same
  // in different records, etc. It checks the depList array.
  void check()
    {
      debug(veryverbose) writefln("Consistancy check on ", this);

      // Check that all dependencies have been resolved. delList is
      // successively sliced down to zero when indices are looked up.
      assert(depList.length == 0);

      debug(statecheck) assert(state++ == 2);
    }

  // Convert an integer record index to an object pointer of type T.
  template lookup(T: Record)
    {
      T lookup(Record[] list)
	{
	  // Get the next dependency from the list
	  int i = depList[0];
	  depList = depList[1..depList.length()];

	  debug(verbose)
	    {
	      T t = lookupCast!(T)(i, list);
	      debug(veryverbose)
		{
		  writef("  Resolved ", i, " to ");
		  if(t is null) writefln("(null)");
		  else writefln(t);
		}
	      return t;		
	    }
	  else
	    return lookupCast!(T)(i, list);
	}
    }

  template lookupList(T: Record)
    {
      void lookupList(Record[] list, T[] deps)
	{
	  foreach(ref T t; deps)
	    t = lookup!(T)(list);
	}
    }

  // Reads an index (int) and adds it to the list of
  // dependencies. These can be returned later by lookup().
  void getIndex()
    {
      depList ~= nifFile.getInt;
      debug(verbose) writefln("Index ", depList[depList.length()-1]);
    }

  int getIndexList()
    {
      int[] l = nifFile.getInts;
      int num;

      // Only add non-null references
      foreach(int i; l)
	if(i != -1)
	  {
	    depList ~= i;
	    num++;
	  }

      //depList ~= l;

      debug(verbose)
	{
	  writef("Index list: ");
	  foreach(int i; l)
	    writef(i, " ");
	  writefln("(%d kept)", num);
	}

      return num;
    }
}

// Lookup with casting and error checking.
template lookupCast(T: Record)
{
  T lookupCast(int i, Record[] list)
    {
      if(i == -1) return null;

      if(i < 0 || i >= list.length)
	nifFile.fail(format("Record index %d out of bounds (got %d records.)",
			    i, list.length));
      
      Record r = list[i];

      if(r is null)
	{
	  nifFile.warn("Referenced an unknown record type");
	  return null;
	}

      T t = cast(T)r;
      if(t is null)
	nifFile.fail("Cannot convert " ~ r.toString() ~ " to " ~
	     (cast(TypeInfo_Class)typeid(T)).info.name);

      return t;
    }
}

template castCheck(T: Record)
{
  bool castCheck(Record r)
    {
      if(r !is null && cast(T)r is null) return true;
      else return false;
    }
}

template castWarn(T: Record)
{
  debug(check)
    {
      void castWarn(Record r)
	{
	  if(castCheck!(T)(r))
	    nifFile.warn("Could not cast " ~ r.toString() ~ " to " ~
		   (cast(TypeInfo_Class)typeid(T)).info.name);
	}
    }
  else
    void castWarn(Record r) {};
}

class NiSkinInstance : Record
{
  NiSkinData data;
  NiNode root;
  NiNode bones[];

 override:
  void read()
    {
      super.read();

      debug(verbose) writef("Skin data ");
      getIndex();

      debug(verbose) writef("Scene root ");
      getIndex();

      debug(verbose) writef("Bone ");
      bones = nifRegion.allocateT!(NiNode)(getIndexList());
    }

  void sortOut(Record[] l)
    {
      super.sortOut(l);
      data = lookup!(NiSkinData)(l);
      root = lookup!(NiNode)(l);
      lookupList!(NiNode)(l,bones);
    }

  void check()
    {
      super.check();
      debug(check) if(data !is null)
	{
	  if(bones.length != data.weights.length)
	    nifFile.warn(format("NiSkinInstance has ", bones.length,
		  " bones, but NiSkinData has ", data.weights.length));

	  /*
	  int meshCount = root.getFirstMesh(r).data.vertices.length/3;

	  foreach(int i, NiSkinData.Weight[] wl; data.weights)
	    {
	      if(bones[i] is null)
		r.warn("Bone object missing!");

	      if(meshCount < data.weights[i].length)
		r.warn(format("More skin weights than vertices in bone %d (%d < %d)",
			      i, meshCount, data.weights[i].length));

	      foreach(ref NiSkinData.Weight w; data.weights[i])
		if(w.vertex >= meshCount)
		  r.warn("Skin weight vertex index out of bounds");
	    }
	  */
	}
    }
}
