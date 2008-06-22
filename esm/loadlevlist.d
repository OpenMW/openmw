/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadlevlist.d) is part of the OpenMW package.

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

module esm.loadlevlist;
import esm.imports;
import esm.loadcrea;

import util.random;

/* Outdated comments:
 *
 * Leveled lists. Since these have identical layout, I only bothered
 * to implement it once.
 *
 * We should later implement the ability to merge leveled lists from
 * several files. 
 *
 * The Item indices can point to several different lists (potions,
 * weapons, armor, etc.) and this is a problem also for npcs,
 * containers and cells. Deal with it in a uniform way. In fact, we
 * won't have to make any difference between the creature and item
 * lists.
 *
 * EDIT 2007.08.18: Looks like DMD is exploding from template forward
 * references again. It works for very small cases, so I assume it
 * should be legal in our case also, and that this is a BUG. I have
 * cut it down and am going to report it. Moving ListItem out of the
 * struct helped, but I get the same problem in other places, where it
 * is not possible to fix. I should probably cut down the other case
 * as well...
 *
 * EDIT 2007.09.09: In Monster we managed to cut down template
 * forwarding with LinkedList by making the list use Value pointers
 * instead of Node pointers. If I do the same for HashTable and move
 * the value to the top, I JUST might be able to work around this DMD
 * bug.
 *
 * UPDATE: Well, it works now, but only if you compile with DMD using
 * the all-files-on-one-line approach. It will not work with DSSS
 * until these bugs are gone.
 */

// Moved here for template bug reasons...
struct _ListItem
{
  Item item; // Definded in records.d
  short level;
}

struct LeveledListT(bool creature)
{
  char[] id;
  LoadState state;

  enum Flags
    {
      AllLevels		= 0x01, // Calculate from all levels <= player
				// level, not just the closest below
				// player.
      Each		= 0x02  // Select a new item each time this
				// list is instantiated, instead of
				// giving several identical items
    }				// (used when a container has more
				// than one instance of one leveled
				// list.)

  alias _ListItem ListItem;

  Flags flags;
  ubyte chanceNone; // Chance that none are selected (0-255??)
  ListItem list[];

  // Get a random creature from this list
  Creature* instCreature(short PCLevel)
    in
    {
      assert(creature);
    }
  body
    {
      int index = instIndex(PCLevel);
      if(index == -1) return null; // No creature was selected

      Creature *c = cast(Creature*)list[index].item.getPtr(ItemType.Creature);
      assert(c != null);
      return c;
    }

  // Get a random item from the list
  Item *instItem(short PCLevel)
    in
    {
      assert(!creature);
    }
  body
    {
      int index = instIndex(PCLevel);
      if(index == -1) return null;

      return &list[index].item;
    }

  // Get a random index in the right range
  int instIndex(short PCLevel)
    {
      int top, bottom, i;
 
      // TODO: Find out if this is indeed correct. 
      // Test if no creature is to be selected
      if(rnd.randInt(0, 255) < chanceNone) return -1;

      // Find the highest item below or equal to the Player level
      for(i=list.length-1; i>=0; i--)
	if(list[i].level <= PCLevel) break;
      top = i;

      // Select none if the player level is too low
      if(top < 0) return -1;

      // Now find the lowest element to select
      if(flags & Flags.AllLevels) bottom = 0;
      else
	{
	  // Find the lowest index i such that item[i] has the same
	  // level as item[top].
	  do { i--; }
	  while(i>=0 && list[i].level == list[top].level);

	  bottom = i+1;
	}

      // Select a random item
      return rnd.randInt(bottom, top);
    }

  void load()
    {with(esFile){
      flags = cast(Flags)getHNUint("DATA");
      chanceNone = cast(ubyte) getHNByte("NNAM");

      if(hasMoreSubs())
	list = getRegion().allocateT!(ListItem)(getHNInt("INDX"));
      else list = null;

      // TODO: Merge the lists here. This can be done simply by adding
      // the lists together, making sure that they are sorted by
      // level. A better way might be to exclude repeated items. Also,
      // some times we don't want to merge lists, just overwrite. Figure
      // out a way to give the user this option.
 
      foreach(ref ListItem l; list)
	{
	  static if(creature)
	    {
	      getSubNameIs("CNAM");
	      l.item = actors.lookup(tmpHString());
	    }
	  else
	    {
	      getSubNameIs("INAM");

	      // Morrowind.esm actually contains an invalid reference,
	      // to "imperial cuirass" in the list
	      // "random_imp_armor". We just ignore it to avoid
	      // irritating warning messages.
	      char[] tmp = tmpHString();
	      if( (tmp != "imperial cuirass") || (id != "random_imp_armor")
		  || (getSpecial() != SpecialFile.Morrowind) )
		l.item = items.lookup(tmp);
	      //l.item = items.lookup(tmpHString());
	    }
	  l.level = getHNShort("INTV");
	}
    }}
}

alias LeveledListT!(false) LeveledItems;
alias LeveledListT!(true) LeveledCreatures;

ListID!(LeveledCreatures) creatureLists;
ListID!(LeveledItems) itemLists;
