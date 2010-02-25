/*
   This file contains some leftovers which have not yet been ported to
   C++.
 */

align(1) struct AMBIStruct
{
  Color ambient, sunlight, fog;
  float fogDensity;

  static assert(AMBIStruct.sizeof == 16);
}

int max(int x, int y)
{ return x>=y?x:y; }

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
CellList cells;
