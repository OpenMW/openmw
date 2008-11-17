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
import monster.monster;

import util.reglist;

import ogre.ogre;
import ogre.bindings;

import sound.audio;

import scene.player;

// Generic version of a "live" object. Do we even need this at all?
// No, I don't think so.
struct GenLive(T)
{
  // Instance of class GameObject or a derived class (depending on
  // object type)
  MonsterObject *obj;
  T *m;

  Placement *getPos()
    {
      assert(obj !is null);
      // This belongs in HACK-land
      return cast(Placement*)obj.getFloatPtr("x");
    }

  float getScale()
    {
      assert(obj !is null);
      return obj.getFloat("scale");
    }
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

// TODO: This is sort of redundant. Eliminate or rework it later.
struct LiveLight
{
  MonsterObject *obj;
  Light *m;

  Placement *getPos()
    {
      assert(obj !is null);
      // This belongs in HACK-land
      return cast(Placement*)obj.getFloatPtr("x");
    }

  float getScale()
    {
      assert(obj !is null);
      return obj.getFloat("scale");
    }

  NodePtr lightNode;
  SoundInstance *loopSound;
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

      // Set up the Monster classes if it's not done already
      setup();
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

      // TODO: Set up landscape system here.
      /*
      with(esFile)
      {
        restoreContext(exCell.land.context, reg);

        // TODO: Not all of these will be present at all times
        readHNExact(,12675, "VNML");
        readHNExact(,4232, "VHGT");
        readHNExact(,81, "WNAM");
        readHNExact(,12675, "VCLR");
        readHNExact(,512, "VTEX");
      }
      */

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

      // TODO: Read this in loadcell.d
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

 private:

  static
    MonsterClass gameObjC;

  void setup()
    {
      if(gameObjC !is null) return;

      gameObjC = MonsterClass.find("GameObject");
    }

  void loadReferences()
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

            MonsterObject *mo;

	    // These should be ordered according to how commonly they
	    // occur.

	    // Static mesh - probably the most common object type
	    if(Static *s = it.getStatic())
	      {
		LiveStatic ls;
		ls.m = s;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		statics.insert(ls);
		stat = true;
	      }
	    // Misc items are also pretty common
	    else if(Misc *m = it.getMisc())
	      {
		LiveMisc ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		miscItems.insert(ls);
	      }
	    // Lights and containers too
	    else if(Light *m = it.getLight())
	      {
		LiveLight ls;
		ls.m = m;
		ls.obj = m.proto.clone();
                mo = ls.obj;

                bool carry = (m.data.flags&Light.Flags.Carry) != 0;
		if(carry)
		  lights.insert(ls);
		else
		  statLights.insert(ls);
	      }
	    else if(Container *c = it.getContainer())
	      {
		LiveContainer ls;
		ls.m = c;
		ls.obj = c.proto.clone();
                mo = ls.obj;
		containers.insert(ls);
		container = true;
	      }
	    // From here on I doubt the order will matter much
	    else if(Door *d = it.getDoor())
	      {
		LiveDoor ls;
		ls.m = d;
		ls.obj = d.proto.clone();
                mo = ls.obj;
		doors.insert(ls);
		door = true;
	      }
	    // Activator?
	    else if(Activator *a = it.getActivator())
	      {
		LiveActivator ls;
		ls.m = a;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		activators.insert(ls);
		activator = true;
	      }
	    // NPC?
	    else if(NPC *n = it.getNPC())
	      {
		LiveNPC ls;
		ls.m = n;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		npcs.insert(ls);
	      }
	    else if(Potion *p = it.getPotion())
	      {
		LivePotion ls;
		ls.m = p;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		potions.insert(ls);
	      }
	    else if(Apparatus *m = it.getApparatus())
	      {
		LiveApparatus ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		appas.insert(ls);
	      }
	    else if(Ingredient *m = it.getIngredient())
	      {
		LiveIngredient ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		ingredients.insert(ls);
	      }
	    else if(Armor *m = it.getArmor())
	      {
		LiveArmor ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		armors.insert(ls);
	      }
	    else if(Weapon *m = it.getWeapon())
	      {
		LiveWeapon ls;
		ls.m = m;
		ls.obj = m.proto.clone();
                mo = ls.obj;
		weapons.insert(ls);
	      }
	    else if(Book *m = it.getBook())
	      {
		LiveBook ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		books.insert(ls);
	      }
	    else if(Clothing *m = it.getClothing())
	      {
		LiveClothing ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		clothes.insert(ls);
	      }
	    else if(Tool *m = it.getPick())
	      {
		LiveTool ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		tools.insert(ls);
	      }
	    else if(Tool *m = it.getProbe())
	      {
		LiveTool ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		tools.insert(ls);
	      }
	    else if(Tool *m = it.getRepair())
	      {
		LiveTool ls;
		ls.m = m;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		tools.insert(ls);
	      }
	    else if(Creature *c = it.getCreature())
	      {
		LiveCreature ls;
		ls.m = c;
		ls.obj = gameObjC.createObject;
                mo = ls.obj;
		creatures.insert(ls);
	      }
	    else if(LeveledCreatures *l = it.getCreatureList)
	      {
		// Create a creature, based on current player level.
		LiveCreature ls;
		ls.m = l.instCreature(playerData.level);
		if(ls.m != null)
		  {
		    ls.obj = gameObjC.createObject; mo = ls.obj;
		    creatures.insert(ls);
		  }
	      }
	    else fail(format("  UNKNOWN REFERENCE! Type ", cast(int)it.i.type));

	    // Now that the object has found it's place, load data
	    // into base.

	    with(*mo)
	      {
		// Scale
		setFloat("scale", getHNOFloat("XSCL", 1.0));

		// Statics only need the position data. Skip the
		// unneeded calls to isNextSub() as an optimization.
		if(stat) goto readpos;

		// An NPC that owns this object (and will get angry if
		// you steal it)
		setString8("owner", getHNOString("ANAM"));
		
		// I have no idea, link to a global variable perhaps?
		setString8("global", getHNOString("BNAM"));

		// ID of creature trapped in a soul gem (?)
		setString8("soul", getHNOString("XSOL"));

		// ?? CNAM has a faction name, might be for
		// objects/beds etc belonging to a faction.
                {
                  char[] cnam = getHNOString("CNAM");
                  setString8("cnam", cnam);

                  // INDX might be PC faction rank required to use the
                  // item? Sometimes is -1.
                  if(cnam.length) setInt("indx", getHNInt("INDX"));
                }

		// Possibly weapon health, number of uses left or
		// weapon magic charge?
                setFloat("xchg", getHNOFloat("XCHG", 0.0));

		// I have no idea, these are present some times, often
		// along with owner (ANAM) and sometimes otherwise. Is
		// NAM9 is always 1?  INTV is usually one, but big for
		// lights. Perhaps something to do with remaining
		// light "charge". I haven't tried reading it as a
		// float in those cases.
                setInt("intv", getHNOInt("INTV", 0));
                setInt("nam9", getHNOInt("NAM9", 0));

		// Present for doors that teleport you to another
		// cell.
		if(door && isNextSub("DODT"))
		  {
		    setBool("teleport", true);

                    // Warning, HACK! Will be fixed when we implement
                    // structs in Monster.
                    Placement *p = cast(Placement*)getFloatPtr("destx");
		    readHExact(p, Placement.sizeof);

                    // Destination cell (optional?)
		    setString8("destCell", getHNOString("DNAM"));
		  }

		if(door || container)
		  {
		    // Lock level (I think)
		    setInt("lockLevel", getHNOInt("FLTV", 0));

		    // For locked doors and containers
		    setString8("key", getHNOString("KNAM"));
		    setString8("trap", getHNOString("TNAM"));
		  }

		if(activator)
		  {
		    // Occurs ONCE in Morrowind.esm, for an activator.
		    setInt("unam", getHNOByte("UNAM", 0));

		    // Occurs in Tribunal.esm, eg. in the cell
		    // "Mournhold, Plaza Brindisi Dorom", where it has
		    // the value 100.
		    setInt("fltv", getHNOInt("FLTV", 0));
		  }

		readpos:

		// Position and rotation of this object within the
		// cell

                // TODO: This is a HACK. It assumes the class variable
                // floats are placed consecutively in memory in the
                // right order. This is true, but is a very bug prone
                // method of doing this. Will fix when Monster gets
                // structs. (See the DODT record above also.)
                Placement *pos = cast(Placement*)getFloatPtr("x");
		readHNExact(pos, Placement.sizeof, "DATA");

		// TODO/FIXME: Very temporary. Set player position at
		// the first door we find.
		if(door && !playerData.posSet)
		  {
		    playerData.posSet = true;
		    playerData.position = *pos;
		  }
              }
          } // End of while(hasMoreSubs)
      }

    } // End of loadReferences()
} // End of class CellData

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
    // heap here. Also, this is only semi-runtime (executed when
    // loading a cell), thus an occational GC slow down is not
    // critical.
    return new CellData(new RegionManager("CELL"));
  }

  void release(CellData r)
  {
    assert(next < list.length);

    r.killCell();
    list[next++] = r;
  }
}
