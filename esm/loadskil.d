/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadskil.d) is part of the OpenMW package.

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

module esm.loadskil;

import esm.imports;

/*
 * Skill information
 */

enum Specialization : int
  {
    Combat	= 0,
    Magic	= 1,
    Stealth	= 2
  }

struct Skill
{
  align(1) struct SKDTstruct
  {
    Attribute attribute;
    Specialization specialization; // 0 - Combat, 1 - Magic, 2 - Stealth
    float useValue[4]; // How much skill improves. Meaning of each
		       // field depends on what skill this is.
  }
  static assert(SKDTstruct.sizeof == 24);

  SKDTstruct data;

  char[] description; // Description

  void load()
    {
      esFile.readHNExact(&data, data.sizeof, "SKDT");
      description = esFile.getHNOString("DESC");
    }
}

class SkillList : ListKeeper
{
  // See SkillEnum in defs.d for a definition of skills
  Skill[SkillEnum.Length] list;
  static assert(list.length == 27);

 override:

  void load()
    {
      int index = esFile.getHNInt("INDX");

      if(index < 0 || index >= list.length)
	esFile.fail("Invalid skill number " ~ .toString(index));

      list[index].load();
    }

  void endFile() {}

  uint length() { return list.length; }

  void *lookup(char[] s) { assert(0); }
}
SkillList skills;
