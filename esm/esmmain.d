/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (esmmain.d) is part of the OpenMW package.

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

module esm.esmmain;

public import esm.records;

/* This file is the main module for loading from ESM, ESP and ESS
   files. It stores all the data in the appropriate data structures
   for later referal. TODO: Put this in a class or whatever? Nah, we
   definately only need one structure like this at any one
   time. However, we have to deal with unloading and reloading it,
   even though that should be the exceptional case (change of plugins,
   etc), not the rule (loading a savegame should not alter the base
   data set, I think, but it's to early do decide.)*/

// Load a set of esm and esp files. For now, we just traverse in the
// order given. Later, we should sort these into 'masters' and
// 'plugins', because esms are always supposed to be loaded
// first. TODO: I'm not sure if I should load all these in one
// function. Do we need to be able to respond to errors in each file?
// Nah, if anything fails, give a general error message, remove the
// file from the list and try again. We have to be able to get a list
// of which files depend upon which, though... this can be done before
// this function is called.
void loadTESFiles(char[][] files)
{
  // Set up all the lists to hold our data
  initializeLists();

  foreach(char[] filename; files)
    {
      esFile.open(filename, esmRegion);
      while(esFile.hasMoreRecs())
	{
	  uint flags;

	  // Read record header
	  char[] recName = esFile.getRecName();
	  esFile.getRecHeader(flags);

	  if(flags & RecordFlags.Unknown)
	    esFile.fail(format("UNKNOWN record flags: %xh", flags));

	  loadRecord(recName);
	}

      // We have to loop through the lists and check for broken
      // references at this point, and if all forward references were
      // loaded. There might be other end-of-file things to do also.
      endFiles();
    }

  esFile.close();

  // Put all inventory items into one list
  items.addList(appas, ItemType.Apparatus);
  items.addList(lockpicks, ItemType.Pick);
  items.addList(probes, ItemType.Probe);
  items.addList(repairs, ItemType.Repair);
  items.addList(lights, ItemType.Light);
  items.addList(ingreds, ItemType.Ingredient);
  items.addList(potions, ItemType.Potion);
  items.addList(armors, ItemType.Armor);
  items.addList(weapons, ItemType.Weapon);
  items.addList(books, ItemType.Book);
  items.addList(clothes, ItemType.Clothing);
  items.addList(miscItems, ItemType.Misc);
  items.addList(itemLists, ItemType.ItemList); // Leveled item lists

  // Same with all actors
  actors.addList(creatures, ItemType.Creature);
  actors.addList(creatureLists, ItemType.CreatureList);
  actors.addList(npcs, ItemType.NPC);

  // Finally, add everything that might be looked up in a cell into
  // one list
  cellRefs.addList(items);
  cellRefs.addList(actors);
  cellRefs.addList(doors, ItemType.Door);
  cellRefs.addList(activators, ItemType.Activator);
  cellRefs.addList(statics, ItemType.Static);
  cellRefs.addList(containers, ItemType.Container);

  // Check that all references are resolved
  items.endMerge();
  actors.endMerge();
  cellRefs.endMerge();

  // Put all NPC dialogues into the hyperlink list
  foreach(char[] id, ref Dialogue dl; dialogues.names)
    hyperlinks.add(id, &dl);

  // Finally, sort the hyperlink lists
  hyperlinks.sort();
}

// Load a TES savegame file (.ess). Currently VERY limited, only reads
// the header.
void importSavegame(char[] file)
{
  writefln("Loading savegame %s", file);
  esFile.open(file, esmRegion);
  scope(exit) esFile.close();

  if(esFile.getFileType != FileType.Ess)
    throw new TES3FileException(file ~ " is not a savegame");

  with(esFile.saveData)
    {
      writefln("Floats:");
      foreach(i, f; unknown)
        writefln("  %s: %s", i, f);

      writefln("Cell name: ", stripz(cell));

      writefln("Strange value: ", unk2);
      writefln("Player name: ", stripz(player));
    }
  writefln();
}
