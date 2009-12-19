/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadfact.d) is part of the OpenMW package.

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

module esm.loadfact;

import esm.imports;

/*
 * Faction definitions
 */

// Requirements for each rank
align(1) struct RankData
{
  int attribute1, attribute2; // Attribute level

  int skill1, skill2;	  // Skill level (faction skills given in
		          // skillID below.) You need one skill at
		          // level 'skill1' and two skills at level
		          // 'skill2' to advance to this rank.

  int factReaction;	  // Reaction from faction members
}

struct Faction
{
  char[] id, name;
  LoadState state;

  align(1) struct FADTstruct
  {
    // Which attributes we like
    int	attribute1, attribute2;

    RankData rankData[10];

    int skillID[6]; // IDs of skills this faction require
    int unknown;    // Always -1?
    uint isHidden;  // 1 - hidden from player

    static assert(RankData.sizeof == 20);
    static assert(FADTstruct.sizeof == 240);
  }

  FADTstruct data;

  RankData rankData[10];

  struct Reaction
  {
    Faction* faction;
    int reaction;
  }

  RegionBuffer!(Reaction) reactions;

  char[] ranks[10];	// Name of faction ranks (may be empty for NPC
			// factions)
  void load()
    {with(esFile){
      name = getHNString("FNAM");

      // Read rank names. These are optional.
      ranks[] = null;
      int i = 0;
      while(isNextSub("RNAM") && i<10) ranks[i++] = getHString();
      if(isNextSub("RNAM")) fail("Too many rank names");

      // Main data struct
      readHNExact(&data, data.sizeof, "FADT");

      if(data.isHidden > 1) fail("Unknown flag!");

      // Read faction response values
      reactions = getRegion().getBuffer!(Reaction)();
      while(hasMoreSubs())
	{
	  Reaction r;
	  r.faction = getHNPtr!(Faction)("ANAM", factions);
	  r.reaction = getHNInt("INTV");
	  reactions ~= r;
	}
    }}
}

ListID!(Faction) factions;
