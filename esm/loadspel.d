/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadspel.d) is part of the OpenMW package.

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

module esm.loadspel;
import esm.imports;

/*
 * Spell
 */

struct SpellList
{
  RegionBuffer!(Spell*) list;

  void load()
  {
    list = esFile.getRegion().getBuffer!(Spell*)(0,1);

    while(esFile.isNextSub("NPCS"))
      list ~= esFile.getHPtr!(Spell)(spells);
  }
}

struct Spell
{
  char[] id;
  LoadState state;

  enum SpellType
    {
      Spell	= 0, // Normal spell, must be cast and costs mana
      Ability	= 1, // Inert ability, always in effect
      Blight	= 2, // Blight disease
      Disease	= 3, // Common disease
      Curse	= 4, // Curse (?)
      Power	= 5  // Power, can use once a day
    }

  enum Flags
    {
      Autocalc	= 1, 
      PCStart	= 2,
      Always	= 4 // Casting always succeeds
    }

  align(1) struct SPDTstruct
  {
    SpellType type;
    int cost;
    Flags flags;

    static assert(SPDTstruct.sizeof==12);
  }

  SPDTstruct data;

  char[] name;
  EffectList effects;

  void load()
    {with(esFile){
      name = getHNOString("FNAM");
      readHNExact(&data, data.sizeof, "SPDT");

      effects = getRegion().getBuffer!(ENAMstruct)(0,1);

      while(isNextSub("ENAM"))
	{
	  effects.length = effects.length + 1;
	  readHExact(&effects.array[$-1], effects.array[$-1].sizeof);
	}
    }}

}

ListID!(Spell) spells;
