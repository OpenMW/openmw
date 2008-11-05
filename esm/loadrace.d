/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadrace.d) is part of the OpenMW package.

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

module esm.loadrace;

import esm.imports;

import esm.loadbsgn; // Defines PowerList

/*
 * Race definition
 */

struct Race
{
  align(1) struct SkillBonus
  {
    SkillEnum skill;
    int bonus;
  }
  static assert(SkillBonus.sizeof == 8);

  enum Flags
    {
      Playable	= 0x01,
      Beast	= 0x02
    }

  align(1) struct RADTstruct
  {
    // List of skills that get a bonus
    SkillBonus bonus[7];

    // Attribute values for male/female
    int[2] strength, intelligence, willpower, agility,
      speed, endurance, personality, luck;

    // The actual eye level height (in game units) is (probably) given
    // as 'height' times 128. This has not been tested yet.
    float[2] height, weight;

    Flags flags; // 0x1 - playable, 0x2 - beast race

    static assert(RADTstruct.sizeof == 140);
  }

  RADTstruct data;

  LoadState state;

  char[] id, name, description;

  SpellList powers;

  void load()
  {with(esFile){
    name = getHNString("FNAM");
    readHNExact(&data, data.sizeof, "RADT");

    powers.load();

    description = getHNOString("DESC");
  }}
}
ListID!(Race) races;
