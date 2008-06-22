/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadmgef.d) is part of the OpenMW package.

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

module esm.loadmgef;

import esm.imports;

import esm.loadweap;
import esm.loadstat;

struct MagicEffect
{
  enum Flags : int
    {
      SpellMaking = 0x0200,
      Enchanting  = 0x0400,
      Negative    = 0x0800 // A harmful effect
    }

  align(1) struct MEDTstruct
  {
    SpellSchool school;
    float baseCost;

    Flags flags;

    int red, blue, green;
    float speed, size, sizeCap; // Noe clue

    static assert(MEDTstruct.sizeof == 36);
  }

  MEDTstruct data;

  IconIndex icon;
  TextureIndex particle;
  Static* casting, hit, area;
  Weapon* bolt;
  Sound* castSound, boltSound, hitSound, areaSound;
  char[] description;

  int index;

  void load()
  {with(esFile){
    readHNExact(&data, data.sizeof,"MEDT");

    icon = getIcon();
    particle = getOTexture("PTEX");

    boltSound = getHNOPtr!(Sound)("BSND", sounds);
    castSound = getHNOPtr!(Sound)("CSND", sounds);
    hitSound = getHNOPtr!(Sound)("HSND", sounds);
    areaSound = getHNOPtr!(Sound)("ASND", sounds);

    casting = getHNOPtr!(Static)("CVFX", statics);
    bolt = getHNOPtr!(Weapon)("BVFX", weapons);
    hit = getHNOPtr!(Static)("HVFX", statics);
    area = getHNOPtr!(Static)("AVFX", statics);

    description = getHNOString("DESC");
  }}
}

class MagicEffectList : ListKeeper
{
  // 0-136 in Morrowind
  // 137 in Tribunal
  // 138-140 in Bloodmoon (also changes 64?)
  // 141-142 are summon effects introduced in bloodmoon, but not used
  // there. They can be redefined in mods by setting the name in GMST
  // sEffectSummonCreature04/05 creature id in
  // sMagicCreature04ID/05ID.
  MagicEffect[143] list;

 override:

  void load()
    {
      int index = esFile.getHNInt("INDX");

      if(index < 0 || index >= list.length)
	esFile.fail("Invalid magic effect number " ~ .toString(index));

      list[index].load();
    }

  void endFile() {}

  uint length() { return list.length; }

  void* lookup(char[] s) { assert(0); }
}
MagicEffectList effects;
