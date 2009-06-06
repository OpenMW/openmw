/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadcell.d) is part of the OpenMW package.

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

module esm.loadcell;

import esm.imports;
import esm.loadregn;

import std.math : abs;

int max(int x, int y)
{ return x>=y?x:y; }

/* Cells can be seen as a collection of several records, and holds
 * data about objects, creatures, statics and landscape (for exterior
 * cells). This data can be huge, and has to be loaded when we need
 * it, not when parsing the file.
 */

align(1) struct AMBIStruct
{
  Color ambient, sunlight, fog;
  float fogDensity;

  static assert(AMBIStruct.sizeof == 16);
}

enum CellFlags : uint
  {
    Interior	= 0x01,
    HasWater	= 0x02,
    NoSleep	= 0x04,
    QuasiExt	= 0x80 // Behave like exterior (tribunal, requires
		       // version 1.3? TODO: Check this (and for other
		       // tribunal-only stuff))
  }

struct InteriorCell
{
  char[] id;
  CellFlags flags;

  LoadState state;

  // Stores file position so we can load the cell later
  TES3FileContext context;

  // This struct also holds a file context for use later
  PathGrid paths;

  void load()
    {with(esFile){
      // Save the file position and skip to the next record (after the CELL)
      getContext(context);
      skipRecord();

      // Check for path grid data
      paths.state = LoadState.Unloaded;
      if(isNextHRec("PGRD")) paths.load();
    }}
}

struct ExteriorCell
{
  // This isn't an id, so we call it 'name' instead. May be empty, if
  // so use region name to display
  char[] name;

  CellFlags flags;
  int gridX, gridY;

  LoadState state;

  Region* region;

  // We currently don't load all cell data (these can be huge!), and
  // it makes sense to only load these when accessed. Use this to
  // reopen the file later.
  TES3FileContext context;

  // Landscape and path grid data
  Land land; // There can be TWO landscapes! Or maybe I have
	     // misunderstood something. Need to check what it means.

  PathGrid paths;

  // Return true if we have land data
  bool hasLand()
  {
    return land.state == LoadState.Loaded && land.hasData;
  }

  void load()
    {with(esFile){
      this.region = getHNOPtr!(Region)("RGNN", regions);

      // Save the file position and skip to the next record (after the CELL)
      getContext(context);
      skipRecord();

      // Exterior cells might have landscape data
      land.state = LoadState.Unloaded;
      if(isNextHRec("LAND")) land.load();

      // Check for path grid data
      paths.state = LoadState.Unloaded;
      if(isNextHRec("PGRD")) paths.load();

      // Land can also be here instead. In fact, it can be both
      // places. I have to figure out what it means.
      if(isNextHRec("LAND")) land.load();

      if(land.state == LoadState.Loaded)
        if(gridX != land.X || gridY != land.Y)
          writefln("WARNING: Grid mismatch at %s,%s!", gridX, gridY);
    }}
}

struct ExtCellHash
{
  // This is a pretty good hash, gives no collisions for all of
  // Morrowind.esm when the table size is 2048, and it gives very few
  // collisions overall. Not that it matters that much.
  static uint hash(uint val)
  {
    uint res = cast(ushort)val;
    res += *(cast(ushort*)&val+1)*41;
    return res;
  }

  static bool isEqual(uint a, uint b) { return a==b; }
}

class CellList : ListKeeper
{
  // Again, these are here to avoid DMD template bugs
  alias _aaNode!(char[], InteriorCell) _unused1;
  alias _aaNode!(uint, ExteriorCell) _unused2;

  HashTable!(char[], InteriorCell, ESMRegionAlloc) in_cells;
  HashTable!(uint, ExteriorCell, ESMRegionAlloc, ExtCellHash) ex_cells;

  // Store the maximum x or y coordinate (in absolute value). This is
  // used in the landscape pregen process.
  int maxXY;

  this()
    {
      in_cells = in_cells.init;
      in_cells.rehash(1600);

      ex_cells = ex_cells.init;
      ex_cells.rehash(1800);
    }

  align(1) struct DATAstruct
  {
    CellFlags flags;
    int gridX, gridY;
    static assert(DATAstruct.sizeof==12);
  }

  DATAstruct data;

  // Look up an interior cell, throws an error if not found (might
  // change later)
  InteriorCell *getInt(char[] s)
    {
      return in_cells.getPtr(s);
    }

  // Exterior cell, same as above
  ExteriorCell *getExt(int x, int y)
    {
      return ex_cells.getPtr(compound(x,y));
    }

  // Check whether we have a given exterior cell
  bool hasExt(int x, int y)
    {
      return ex_cells.inList(compound(x,y));
    }

  void *lookup(char[] s)
    { assert(0); }

  void endFile()
    out
    {
      in_cells.validate();
      ex_cells.validate();
    }
    body
    {
      foreach(id, ref c; in_cells)
	{
	  if(c.state == LoadState.Loaded) c.state = LoadState.Previous;
	  // We never forward reference cells!
	  assert(c.state != LoadState.Unloaded);
	}

      foreach(id, ref c; ex_cells)
	{
	  if(c.state == LoadState.Loaded) c.state = LoadState.Previous;
	  // We never forward reference cells!
	  assert(c.state != LoadState.Unloaded);
	}
    }

  uint length() { return numInt() + numExt(); }
  uint numInt() { return in_cells.length; }
  uint numExt() { return ex_cells.length; }

  // Turn an exterior cell grid position into a unique number
  static uint compound(int gridX, int gridY)
    {
      return cast(ushort)gridX + ((cast(ushort)gridY)<<16);
    }

  static void decompound(uint val, out int gridX, out int gridY)
    {
      gridX = cast(short)(val&0xffff);
      gridY = cast(int)(val&0xffff0000) >> 16;
    }

  void load()
    {with(esFile){
      char[] id = getHNString("NAME");

      // Just ignore this, don't know what it does. I assume it
      // deletes the cell, but we can't handle that yet.
      if(isNextSub("DELE")) getHInt();

      readHNExact(&data, data.sizeof, "DATA");

      if(data.flags & CellFlags.Interior)
	{
	  InteriorCell *p;
	  if(in_cells.insertEdit(id, p))
	    // New item was inserted
	    {
	      p.state = LoadState.Unloaded;
	      p.id = id;
	      p.flags = data.flags;
	      p.load();
	      p.state = LoadState.Loaded;
	    }
	  else
	    // Overloading an existing cell
	    {
	      if(p.state != LoadState.Previous)
		fail("Duplicate interior cell " ~ id);

	      assert(id == p.id);
	      p.load();
	      p.state = LoadState.Loaded;
	    }
	}
      else // Exterior cell
	{
	  uint key = compound(data.gridX, data.gridY);

	  ExteriorCell *p;
	  if(ex_cells.insertEdit(key, p))
	    // New cell
	    {
	      p.state = LoadState.Unloaded;
	      p.name = id;
	      p.flags = data.flags;
	      p.gridX = data.gridX;
	      p.gridY = data.gridY;
	      p.load();
	      p.state = LoadState.Loaded;

              int mx = max(abs(p.gridX), abs(p.gridY));
              maxXY = max(maxXY, mx);
	    }
	  else
	    {
	      if(p.state != LoadState.Previous)
		fail(format("Duplicate exterior cell %d %d",
			    data.gridX, data.gridY));
	      assert(p.gridX == data.gridX);
	      assert(p.gridY == data.gridY);
	      p.load();
	      p.state = LoadState.Loaded;
	    }
	}
    }}
}

/*
 * Landscape data.
 */

struct Land
{
  LoadState state;

  uint flags; // ?? - only first four bits seem to be used

  // Map coordinates.
  int X, Y;

  TES3FileContext context;

  bool hasData = false;

  void load()
  {with(esFile){
    // Get the grid location
    ulong grid = getHNUlong("INTV");
    X = grid & uint.max;
    Y = (grid >> 32);

    flags = getHNInt("DATA");

    // Save file position
    getContext(context);

    int cnt;

    // Skip these here. Load the actual data when the cell is loaded.
    if(isNextSub("VNML")) {skipHSubSize(12675);cnt++;}
    if(isNextSub("VHGT")) {skipHSubSize(4232);cnt++;}
    if(isNextSub("WNAM")) skipHSubSize(81);
    if(isNextSub("VCLR")) skipHSubSize(12675);
    if(isNextSub("VTEX")) {skipHSubSize(512);cnt++;}

    // We need all three of VNML, VHGT and VTEX in order to use the
    // landscape.
    hasData = (cnt == 3);

    if(state == LoadState.Loaded)
      writefln("Duplicate landscape data in " ~ getFilename());
    state = LoadState.Loaded;
  }}
}

/*
 * Path grid.
 */
struct PathGrid
{
  struct DATAstruct
  {
    int x, y; // Grid location, matches cell for exterior cells
    short s1; // ?? Usually but not always a power of 2. Doesn't seem
	      // to have any relation to the size of PGRC.
    short s2; // Number of path points? Size of PGRP block is always 16 * s2;
  }

  DATAstruct data;

  TES3FileContext context;

  LoadState state;

  void load()
  {with(esFile){
    assert(state == LoadState.Unloaded);

    readHNExact(&data, data.sizeof, "DATA");
    getHNString("NAME"); // Cell name, we don't really need it so just
                         // ignore it.

    // Remember this file position
    getContext(context);

    // Check that the sizes match up. Size = 16 * s2 (path points?)
    if(isNextSub("PGRP"))
      {
	int size = skipHSub();
	if(size != 16*data.s2)
	  fail("Path grid table size mismatch");
      }

    // Size varies. Path grid chances? Connections? Multiples of 4
    // suggest either int or two shorts, or perhaps a float. Study
    // it later.
    if(isNextSub("PGRC"))
      {
	int size = skipHSub();
	if(size % 4 != 0)
	  fail("PGRC size not a multiple of 4");
      }

    state = LoadState.Loaded;
  }}
}
CellList cells;
