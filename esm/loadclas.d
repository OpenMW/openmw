/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadclas.d) is part of the OpenMW package.

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
module esm.loadclas;

import esm.imports;

import esm.loadskil;

/*
 * Character class definitions
 */

// These flags tells us which items should be auto-calculated for this
// class
struct Class
{
  enum AutoCalc : uint
    {
      Weapon		= 0x00001,
      Armor		= 0x00002,
      Clothing	 	= 0x00004,
      Books		= 0x00008,
      Ingredient	= 0x00010,
      Lockpick		= 0x00020,
      Probe		= 0x00040,
      Lights		= 0x00080,
      Apparatus		= 0x00100,
      Repair		= 0x00200,
      Misc		= 0x00400,
      Spells		= 0x00800,
      MagicItems	= 0x01000,
      Potions		= 0x02000,
      Training		= 0x04000,
      Spellmaking	= 0x08000,
      Enchanting	= 0x10000,
      RepairItem	= 0x20000
    }

  align(1) struct CLDTstruct
  {
    Attribute attribute[2]; // Attributes that get class bonus

    Specialization specialization; // 0 = Combat, 1 = Magic, 2 = Stealth

    SkillEnum[2][5] skills; // Minor and major skills.

    uint isPlayable; // 0x0001 - Playable class

    // I have no idea how to autocalculate these items...
    AutoCalc calc;

    static assert(CLDTstruct.sizeof == 60);
  }

  LoadState state;
  char[] id, name, description;
  CLDTstruct data;

  void load()
    {
      name = esFile.getHNString("FNAM");
      esFile.readHNExact(&data, data.sizeof, "CLDT");

      if(data.isPlayable > 1)
	esFile.fail("Unknown bool value");

      description = esFile.getHNOString("DESC");
    }
}
ListID!(Class) classes;
