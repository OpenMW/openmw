/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadcont.d) is part of the OpenMW package.

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

module esm.loadcont;
import esm.imports;

/*
 * Container definition
 */

struct ContItem
{
  Item item;
  int count;
}

struct InventoryList
{
  RegionBuffer!(ContItem) list;

  private static char[32] buffer;

  void load()
  {
    list = esFile.getRegion().getBuffer!(ContItem)(0,10);
    
    while(esFile.isNextSub("NPCO"))
      {
	esFile.getSubHeaderIs(36);
	list.length = list.length + 1;

	with(list.array[$-1])
	  {
	    esFile.getInt(count);

	    // String is automatically chopped
	    item = items.lookup(esFile.getString(buffer));
	  }
      }
  }	
}

struct Container
{
  enum Flags
    {
      Organic	= 1, // Objects cannot be placed in this container
      Respawn	= 2, // Respawns after 4 months
      Unknown	= 8
    }

  char[] id, name;
  LoadState state;

  MeshIndex model;
  Script *script;

  float weight; // Not sure, might be max total weight allowed?
  Flags flags;
  InventoryList inventory;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNString("FNAM");
      weight = getHNFloat("CNDT");
      flags = cast(Flags)getHNUint("FLAG");

      if(flags & 0xf4) fail("Unknown flags");
      if(!(flags & 0x8)) fail("Flag 8 not set");

      /*
      if(getFileType == FileType.Savegame)
	{
	  int tmp = getHNInt("INDX");
	  if(tmp) writefln("WARNING, INDX != 0: ", tmp);
	}
      */

      script = getHNOPtr!(Script)("SCRI", scripts);

      inventory.load();
    }}
}

ListID!(Container) containers;
