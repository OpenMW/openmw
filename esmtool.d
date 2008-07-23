/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (esmtool.d) is part of the OpenMW package.

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

module esmtool;

import std.stdio;

import core.memory;
import esm.esmmain;
import monster.util.string;

import std.gc;
import gcstats;


// Not used, but we have to link it in along with the C++ stuff.
import input.events;

void poolSize()
{
  GCStats gc;
  getStats(gc);
  writefln("Pool size: ", comma(gc.poolsize));
  writefln("Used size: ", comma(gc.usedsize));
}

//alias int[Dialogue*] TopicList;

void main(char[][] args)
{
  char[][] files;
  bool raw;

  bool scptList; // List scripts
  bool scptShow; // Show a script
  char[] scptName; // Script to show

  bool ciList; // List interior cells
  bool ceList; // List exterior cells that have names

  bool weList; // List weapons

  bool numbers; // List how many there are of each record type

  foreach(char[] a; args[1..$])
    if(a == "-r") raw = true;

    else if(a == "-sl") scptList = true;
    else if(a == "-s") scptShow = true;
    else if(scptShow && scptName == "") scptName = a;

    else if(a == "-cil") ciList = true;
    else if(a == "-cel") ceList = true;

    else if(a == "-wl") weList = true;

    else if(a == "-n") numbers = true;

    else if(a.begins("-")) writefln("Ignoring unknown option %s", a);
    else files ~= a;

  int help(char[] msg)
    {
      writefln("%s", msg);
      writefln("Syntax: %s [options] esm-file [esm-file ... ]", args[0]);
      writefln("  Options:");
      writefln("    -r            Display all records in raw format");
      writefln("    -n            List the number of each record");
      writefln("    -sl           List scripts");
      writefln("    -s name       Show given script");
      writefln("    -cil          List interior cells");
      writefln("    -cel          List exterior cells with names");
      writefln("    -wl           List weapons");
      return 1;
    }
  if(files.length == 0) return help("No input files given");

  if(scptShow && scptName == "") return help("No script name given");

  initializeMemoryRegions();

  if(raw)
    {
      foreach(int fileNum, char[] filename; files)
	{
	  try
	    {
	      esFile.open(filename, esmRegion);
	      printRaw();
	    }
	  catch(Exception e)
	    {
	      try {writefln(e);}
	      catch {}
	      writefln("Error on file %s", filename);
	    }
	  catch
	    {
	      writefln("Error: Unkown failure on file %s", filename);
	    }
	}
      return;
    }

  // Disable resource lookups.
  resources.dummy = true;

  try loadTESFiles(files);
  catch(Exception e)
    {
      writefln(e);
    }
  catch { writefln("Error: Unkown failure"); }

  // List weapons
  if(weList) foreach(n, m; weapons.names)
    {
      alias Weapon.Type WT;
      switch(m.data.type)
	{
	case WT.ShortBladeOneHand: writef("Short Sword"); break;
	case WT.LongBladeOneHand: writef("Long Sword, One-Handed"); break;
	case WT.LongBladeTwoHand: writef("Long Sword, Two-Handed"); break;
	case WT.BluntOneHand: writef("Blunt, One-Handed"); break;
	case WT.BluntTwoClose: writef("Blunt, Two-Handed"); break;
	case WT.BluntTwoWide: writef("Blunt, Two-Handed Wide"); break;
	case WT.SpearTwoWide: writef("Spear, Two-Handed"); break;
	case WT.AxeOneHand: writef("Axe, One-Handed"); break;
	case WT.AxeTwoHand: writef("Axe, Two-Handed"); break;
	case WT.MarksmanBow: writef("Bow"); break;
	case WT.MarksmanCrossbow: writef("Crossbow"); break;
	case WT.MarksmanThrown: writef("Thrown weapon"); break;
	case WT.Arrow: writef("Arrow"); break;
	case WT.Bolt: writef("Bolt"); break;
        default: assert(0);
	}
      writefln(" id '%s': name '%s'", n, m.name);

      if(m.data.flags & Weapon.Flags.Magical)
	writefln("Magical");
      if(m.data.flags & Weapon.Flags.Silver)
	writefln("Silver");

      writefln("Weight: ", m.data.weight);
      writefln("Value: ", m.data.value);
      writefln("Health: ", m.data.health);
      writefln("Speed: ", m.data.speed);
      writefln("Reach: ", m.data.reach);
      writefln("Enchantment points: ", m.data.enchant);
      writefln("Combat: ", m.data.chop, m.data.slash, m.data.thrust);

      if(m.enchant) writefln("Has enchantment '%s'", m.enchant.id);
      if(m.script) writefln("Has script '%s'", m.script.id);

      writefln();
    }

  if(numbers)
    {
      writefln("Activators: ", activators.length);
      writefln("Doors: ", doors.length);
      writefln("Globals: ", globals.length);
      writefln("Sounds: ", sounds.length);
      writefln("Game Settings: ", gameSettings.length);
      writefln("Factions: ", factions.length);	
      writefln("Statics: ", statics.length);
      writefln("Spells: ", spells.length);
      writefln("Potions: ", potions.length);
      writefln("Apparatus: ", appas.length);
      writefln("Armors: ", armors.length);
      writefln("Body parts: ", bodyParts.length);
      writefln("Enchantments: ", enchants.length);
      writefln("Books: ", books.length);
      writefln("Birth signs: ", birthSigns.length);
      writefln("Land texture files: ", landTextures.length);
      writefln("Weapons: ", weapons.length);
      writefln("Lockpicks: ", lockpicks.length);
      writefln("Probes: ", probes.length);
      writefln("Repairs: ", repairs.length);
      writefln("Cells: ", cells.length);
      writefln("  Interior: ", cells.numInt);
      writefln("  Exterior: ", cells.numExt);
      writefln("Regions: ", regions.length);
      writefln("Lights: ", lights.length);
      writefln("Skills: ", skills.length);
      writefln("Sound generators: ", soundGens.length);
      writefln("Races: ", races.length);
      writefln("Misc items: ", miscItems.length);
      writefln("Cloths: ", clothes.length);
      writefln("Ingredients: ", ingreds.length);
      writefln("Classes: ", classes.length);
      writefln("Containers: ", containers.length);
      writefln("Creatures: ", creatures.length);
      writefln("Leveled item lists: ", itemLists.length);
      writefln("Leveled creature lists: ", creatureLists.length);
      writefln("NPCs: ", npcs.length);
      writefln("Scripts: ", scripts.length);
      writefln("Dialogues: ", dialogues.length);
      writefln("Hyperlinks: ", hyperlinks.list.length);
      writefln("Start scripts: ", startScripts.length);
      writefln("\nTotal items: ", items.length);
      writefln("Total actors: ", actors.length);
      writefln("Total cell placable items: ", cellRefs.length);
    }
  if(scptList) foreach(a, b; scripts.names) writefln(a);
  if(ciList)
    foreach(a, b; cells.in_cells)
      writefln(a);
  if(ceList)
    foreach(uint i, c; .cells.ex_cells)
    {
      int x, y;
      CellList.decompound(i, x, y);
      if(c.name.length)
        writefln("%s,%s: %s", x, y, c.name);
    }

  if(scptShow)
    {
      Script *p = scripts.names.lookup(scptName);
      if(p)
	writefln("Script '%s', text is:\n-------\n%s\n-------", p.id, p.scriptText);
      else writefln("Script '%s' not found", scptName);
      writefln();
    }

  writefln(esmRegion);

  poolSize();
}

// Quick function that simply iterates through an ES file and prints
// out all the records and subrecords. Some of this code is really old
// (about 2004-2005)
void printRaw()
{
  with(esFile)
    {
      // Variable length integer (this only works for unsigned ints!)
      ulong getHVUint()
	{
	  ulong l;

	  getSubHeader();
	  if( (getSubSize != 4) &&
	      (getSubSize != 2) &&
	      (getSubSize != 8) )
	    fail(format("Unknown integer size: ", getSubSize));

	  readExact(&l, getSubSize);
	  return l;
	}

      writefln("Filename: ", getFilename);
      writef("Filetype: ");
      switch(getFileType())
	{
	case FileType.Plugin: writefln("Plugin"); break;
	case FileType.Master: writefln("Master"); break;
	case FileType.Savegame: writefln("Savegame"); break;
	case FileType.Unknown: writefln("Unknown"); break;
        default: assert(0);
	}
      writef("Version: ");
      if(isVer12()) writefln("1.2");
      else if(isVer13()) writefln("1.3");
      else writefln("Unknown");
      
      writefln("Records: ", getRecords);
      writefln("Master files:");
      for(int i; i<getMasters.length; i++)
	writefln("  %s", getMasters[i].name, ", ", getMasters[i].size, " bytes");
      
      writefln("Author: %s", getAuthor);
      //writefln("Description: %s", desc);
      writefln("Total file size: %d\n", getFileSize);
      
      writefln("List of records:");

      while(hasMoreRecs())
	{
	  uint flags;
	  
	  // Read record header
	  char[] recName = getRecName();
	  getRecHeader(flags);
	  
	  if(flags)
	    {
	      writef("Flags: ");
	      if(flags & RecordFlags.Persistent) writef("Persistent ");
	      if(flags & RecordFlags.Blocked) writef("Blocked ");
	      if(flags & RecordFlags.Flag6) writef("Flag6 ");
	      if(flags & RecordFlags.Flag13) writef("Flag13 ");
	      writefln();
	      if(flags & RecordFlags.Unknown)
		writefln("UNKNOWN flags are set: %xh", flags);
	    }	
	  
	  // Process sub record
	  writef("%s %d bytes", recName, getRecLeft());
	  writefln();
	  while(hasMoreSubs())
	    {
	      getSubName();
	      char[] subName = retSubName();
	      writef("   %s = ", subName);

	      // Process header
	      if(subName == "NAME" || subName == "STRV" ||
		 subName == "FNAM" || subName == "MODL" ||
		 subName == "SCRI" || subName == "RGNN" ||
		 subName == "BNAM" || subName == "ONAM" ||
		 subName == "INAM" || subName == "SCVR" ||
		 subName == "RNAM" || subName == "DNAM" ||
		 subName == "ANAM")
		//subName == "SCTX") // For script text
		//getHString();
		{
		  writefln("'%s'", getHString());
		}
	      else if(subName == "FLTV" || subName == "XSCL") writefln(getHFloat());
	      else if(subName == "INTV" /*|| subName == "NAM0"*/ || subName == "FRMR")
		writefln(getHVUint());
	      else
		{
		  int left = skipHSub();
		  writefln(left, " bytes");
		}
	    }
	  writefln();
	}
    }
}

