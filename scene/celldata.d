/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (celldata.d) is part of the OpenMW package.

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

module scene.celldata;

import std.stdio;

public import esm.esmmain;

import core.memory;

import util.reglist;

import ogre.ogre;
import ogre.bindings;

import sound.audio;

import scene.player;

// Base properties common to all live objects. Currently extremely
// sketchy.
struct LiveObjectBase
{
  // Should this stuff be in here?
  bool disabled; // Disabled in game
  bool deleted; // Deleted relative to plugin file

  // Used for objects created in-game, like custom potions or
  // enchanted items. These can be completely deleted. It is also used
  // for creatures created from leveled lists.
  bool transient;

  // Is this a door that teleports to another cell?
  bool teleport;

  // Scale, 1.0 is normal.
  float scale;

  // Owner of an object / activator
  char[] owner;

  // A global variable? Don't know what it's used for.
  char[] global;

  // Reference to a soul trapped creature?
  char[] soul;

  // Faction owner? Rank?
  char[] cnam;
  int indx;

  // Magic value / health / uses of an item?
  float xchg;

  // ?? See comment below
  int intv, nam9;

  // Destination for a door
  Placement destPos;
  char[] destCell;

  // Lock level?
  int fltv;

  // For locked doors and containers
  char[] key, trap;

  // Position in 3D world
  Placement pos;

  // ??
  byte unam;

  // Don't even get me started on script-related issues
}

// Generic version of a "live" object
struct GenLive(T)
{
  // This HAS to be a pointer, since we load the LOB after copying the
  // LiveWhatever into the linked list.
  LiveObjectBase *base;
  T *m;
}

alias GenLive!(Static) LiveStatic;
alias GenLive!(NPC) LiveNPC;
alias GenLive!(Activator) LiveActivator;
alias GenLive!(Potion) LivePotion;
alias GenLive!(Apparatus) LiveApparatus;
alias GenLive!(Ingredient) LiveIngredient;
alias GenLive!(Armor) LiveArmor;
alias GenLive!(Weapon) LiveWeapon;
alias GenLive!(Book) LiveBook;
alias GenLive!(Clothing) LiveClothing;
alias GenLive!(Tool) LiveTool;
alias GenLive!(Creature) LiveCreature;
alias GenLive!(Door) LiveDoor;
alias GenLive!(Misc) LiveMisc;
alias GenLive!(Container) LiveContainer;

struct LiveLight
{
  LiveObjectBase *base;
  Light *m;

  NodePtr lightNode;
  SoundInstance *loopSound;

  // Lifetime left, in seconds?
  float time;
}

class CellData
{
 private:
  RegionManager reg;

 public:

  InteriorCell *inCell;
  ExteriorCell *exCell;

  // Ambient light data
  AMBIStruct ambi;

  // Water height
  int water;

  // Linked lists that hold references to all the objects in the cell
  RegionList!(LiveStatic) statics;
  RegionList!(LiveMisc) miscItems;
  RegionList!(LiveLight) lights;
  RegionList!(LiveLight) statLights;
  RegionList!(LiveNPC) npcs;
  RegionList!(LiveContainer) containers;
  RegionList!(LiveDoor) doors;
  RegionList!(LiveActivator) activators;
  RegionList!(LivePotion) potions;
  RegionList!(LiveApparatus) appas;
  RegionList!(LiveIngredient) ingredients;
  RegionList!(LiveArmor) armors;
  RegionList!(LiveWeapon) weapons;
  RegionList!(LiveBook) books;
  RegionList!(LiveTool) tools;
  RegionList!(LiveClothing) clothes;
  RegionList!(LiveCreature) creatures;

  this(RegionManager r)
    {
      reg = r;
      killCell(); // Make sure all data is initialized.
    }

  // Kills all data and initialize the object for reuse.
  void killCell()
  {
    inCell = null;
    exCell = null;

    // Reset the lists
    statics.init(reg);
    miscItems.init(reg);
    lights.init(reg);
    statLights.init(reg);
    npcs.init(reg);
    containers.init(reg);
    doors.init(reg);
    activators.init(reg);
    potions.init(reg);
    appas.init(reg);
    ingredients.init(reg);
    armors.init(reg);
    weapons.init(reg);
    books.init(reg);
    tools.init(reg);
    clothes.init(reg);
    creatures.init(reg);

    // Write some statistics
    //writefln(reg);

    reg.freeAll();
  }

  // Load an exterior cell
  void loadExtCell(int x, int y)
    {
      exCell = cells.getExt(x, y);

      writefln("Name: %s", exCell.name);
      writefln("Region: %s", exCell.region.id);

      esFile.restoreContext(exCell.context, reg);

      // Always for exterior cells
      water = 0;

      Color mapColor;
      if(esFile.isNextSub("NAM5"))
	esFile.readHExact(&mapColor, mapColor.sizeof);

      loadReferences();

      const float cellWidth = 8192;

      // TODO/FIXME: This is temporary
      playerData.position.position[0] = x*cellWidth;
      playerData.position.position[1] = y*cellWidth;
      playerData.position.position[2] = 6000;
      playerData.position.rotation[] = 0;
    }

  // Load an interior cell
  void loadIntCell(char[] cName)
    {
      inCell = cells.getInt(cName);
      /*
	writefln("Cell id: '%s'", cName);
	writefln("Cell name: '%s'", cell.id);
      */

      esFile.restoreContext(inCell.context, reg);

      // TODO: Read this crap in loadcell.d
      if(esFile.isNextSub("INTV") || esFile.isNextSub("WHGT"))
	water = esFile.getHInt();
      //writefln("Water height: ", water);

      if(inCell.flags & CellFlags.QuasiExt)
	{
	  Region* reg = esFile.getHNOPtr!(Region)("RGNN", regions);
	  if(reg) writefln("Cell has region %s", reg.id);
	  else writefln("No region");
	  // Determine weather from this region
	}
      else
	{
	  // Only for interior cells
	  esFile.readHNExact(&ambi, ambi.sizeof, "AMBI");
	}

      /*
	writefln("Ambient light: ", ambi.ambient.array);
	writefln("Sunlight:      ", ambi.sunlight.array);
	writefln("Fog color:     ", ambi.ambient.array);
	writefln("Fog density:   ", ambi.fogDensity);
      */
      loadReferences();
    }

  private void loadReferences()
    {
      with(esFile)
      {

	// Now read all the references
	while(hasMoreSubs)
	  {
	    // Number of references in the cell? Maximum once in each
	    // cell, but not always at the beginning, and not always
	    // right. In other words, completely useless. Strange
	    // people...
	    getHNOInt("NAM0", 0);

	    int refnum = getHNInt("FRMR");     // Reference number
	    char[] refr = getHNString("NAME");  // ID of object

	    // Used internally for optimizing
	    bool container;
	    bool door;
	    bool stat;
	    bool activator;

	    // Identify the referenced object by looking up the id
	    // string 'ref' in the global database of all
	    // cell-referencable objects.
	    Item itm = cellRefs.lookup(refr);
            ItemT it = ItemT(itm);

	    // Create a new base object that holds all our reference
	    // data.
	    LiveObjectBase *base = reg.newT!(LiveObjectBase)();

	    // These should be ordered according to how commonly they
	    // occur and how large the reference lists are.

	    // Static mesh - probably the most common
	    if(Static *s = it.getStatic())
	      {
		LiveStatic ls;
		ls.m = s;
		ls.base = base;
		statics.insert(ls);
		stat = true;
	      }
	    // Misc items are also pretty common
	    else if(Misc *m = it.getMisc())
	      {
		LiveMisc ls;
		ls.m = m;
		ls.base = base;
		miscItems.insert(ls);
	      }
	    // Lights and containers too
	    else if(Light *m = it.getLight())
	      {
		LiveLight ls;
		ls.m = m;
		ls.base = base;

		ls.time = m.data.time;

		if(m.data.flags&Light.Flags.Carry)
		  lights.insert(ls);
		else
		  statLights.insert(ls);
	      }
	    else if(Container *c = it.getContainer())
	      {
		LiveContainer ls;
		ls.m = c;
		ls.base = base;
		containers.insert(ls);
		container = true;
	      }
	    // From here on I doubt the order will matter much
	    else if(Door *d = it.getDoor())
	      {
		LiveDoor ls;
		ls.m = d;
		ls.base = base;
		doors.insert(ls);
		door = true;
	      }
	    // Activator?
	    else if(Activator *a = it.getActivator())
	      {
		LiveActivator ls;
		ls.m = a;
		ls.base = base;
		activators.insert(ls);
		activator = true;
	      }
	    // NPC?
	    else if(NPC *n = it.getNPC())
	      {
		LiveNPC ls;
		ls.m = n;
		ls.base = base;
		npcs.insert(ls);
	      }
	    else if(Potion *p = it.getPotion())
	      {
		LivePotion ls;
		ls.m = p;
		ls.base = base;
		potions.insert(ls);
	      }
	    else if(Apparatus *m = it.getApparatus())
	      {
		LiveApparatus ls;
		ls.m = m;
		ls.base = base;
		appas.insert(ls);
	      }
	    else if(Ingredient *m = it.getIngredient())
	      {
		LiveIngredient ls;
		ls.m = m;
		ls.base = base;
		ingredients.insert(ls);
	      }
	    else if(Armor *m = it.getArmor())
	      {
		LiveArmor ls;
		ls.m = m;
		ls.base = base;
		armors.insert(ls);
	      }
	    else if(Weapon *m = it.getWeapon())
	      {
		LiveWeapon ls;
		ls.m = m;
		ls.base = base;
		weapons.insert(ls);
	      }
	    else if(Book *m = it.getBook())
	      {
		LiveBook ls;
		ls.m = m;
		ls.base = base;
		books.insert(ls);
	      }
	    else if(Clothing *m = it.getClothing())
	      {
		LiveClothing ls;
		ls.m = m;
		ls.base = base;
		clothes.insert(ls);
	      }
	    else if(Tool *m = it.getPick())
	      {
		LiveTool ls;
		ls.m = m;
		ls.base = base;
		tools.insert(ls);
	      }
	    else if(Tool *m = it.getProbe())
	      {
		LiveTool ls;
		ls.m = m;
		ls.base = base;
		tools.insert(ls);
	      }
	    else if(Tool *m = it.getRepair())
	      {
		LiveTool ls;
		ls.m = m;
		ls.base = base;
		tools.insert(ls);
	      }
	    else if(Creature *c = it.getCreature())
	      {
		LiveCreature ls;
		ls.m = c;
		ls.base = base;
		creatures.insert(ls);
	      }
	    else if(LeveledCreatures *l = it.getCreatureList)
	      {
		// Create a creature, based on current player level.
		LiveCreature ls;
		ls.m = l.instCreature(playerData.level);
		if(ls.m != null)
		  {
		    ls.base = base;
		    creatures.insert(ls);
		  }
	      }
	    else fail(format("  UNKNOWN REFERENCE! Type ", cast(int)it.i.type));

	    // Now that the object has found it's place, load data
	    // into base.

	    with(*base)
	      {
		// ALL variables must be initialized here
		disabled = false;
		deleted = false;
		transient = false;
		teleport = false;

		// Scale
		scale = getHNOFloat("XSCL", 1.0);

		// Statics only need the position data. Skip the
		// unneeded calls to isNextSub() as an optimization.
		if(stat) goto readpos;

		// An NPC that owns this object (and will get angry if
		// you steal it)
		owner = getHNOString("ANAM");
		
		// ??? I have no idea, link to a global variable
		global = getHNOString("BNAM");

		// ID of creature trapped in a soul gem (?)
		soul = getHNOString("XSOL");

		// ?? CNAM has a faction name, might be for
		// objects/beds etc belonging to a faction.
		cnam = getHNOString("CNAM");

		// INDX might be PC faction rank required to use the
		// item? Sometimes is -1.
		if(cnam.length) indx = getHNInt("INDX");

		// Possibly weapon health, number of uses left or
		// weapon magic charge?
		xchg = getHNOFloat("XCHG", 0.0);

		// I have no idea, these are present some times, often
		// along with owner (ANAM) and sometimes otherwise. Is
		// NAM9 is always 1?  INTV is usually one, but big for
		// lights. Perhaps something to do with remaining
		// light "charge". I haven't tried reading it as a
		// float in those cases.
		intv = getHNOInt("INTV", 0);
		nam9 = getHNOInt("NAM9", 0);

		// Present for doors that teleport you to another
		// cell.
		if(door && isNextSub("DODT"))
		  {
		    teleport = true;
		    readHExact(&destPos, destPos.sizeof);

		    // Destination cell (optitional?)
		    destCell = getHNOString("DNAM");
		  }

		if(door || container)
		  {
		    // Lock level (I think)
		    fltv = getHNOInt("FLTV", 0);

		    // For locked doors and containers
		    key = getHNOString("KNAM");
		    trap = getHNOString("TNAM");
		  }

		if(activator)
		  {
		    // Occurs ONCE in Morrowind.esm, for an activator.
		    unam = getHNOByte("UNAM", 0);

		    // Occurs in Tribunal.esm, eg. in the cell
		    // "Mournhold, Plaza Brindisi Dorom", where it has
		    // the value 100.
		    fltv = getHNOInt("FLTV", 0);
		  }

		readpos:
		// Position of this object within the cell
		readHNExact(&pos, Placement.sizeof, "DATA");

		// TODO/FIXME: Very temporary. Set player position at
		// the first door we find.
		if(door && !playerData.posSet)
		  {
		    playerData.posSet = true;
		    playerData.position = pos;
		  }
	      }
	  }

	// Skip this? Large chance that the same file will be used for
	// the next cell load, and the rest of our system can handle
	// that without closing or reopening the file.

	//close(); // Close the file
      }
    }
}

CellFreelist cellList;

// Structure used as a free list for cell data objects and their
// respective regions.
struct CellFreelist
{
  // We love static arrays! And 100 cells should be enough for
  // everybody :)
  CellData[100] list;
  uint next;

  // TODO: Figure out a good size to use here as well.
  CellData get()
  {
    if(next) return list[--next];

    // Since these are reused, there's no waste in allocating on the
    // stack here. Also, this is only semi-runtime (executed when
    // loading a cell), thus a rare GC slow down is non-critical.
    return new CellData(new RegionManager("CELL"));
  }

  void release(CellData r)
  {
    assert(next < list.length);

    r.killCell();
    list[next++] = r;
  }
}
