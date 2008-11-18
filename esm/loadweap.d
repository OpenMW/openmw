/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadweap.d) is part of the OpenMW package.

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

module esm.loadweap;
import esm.imports;

/*
 * Weapon definition
 */

struct Weapon
{
  enum Type : short
    {
      ShortBladeOneHand	= 0,
      LongBladeOneHand	= 1,
      LongBladeTwoHand	= 2,
      BluntOneHand	= 3,
      BluntTwoClose	= 4,
      BluntTwoWide	= 5,
      SpearTwoWide	= 6,
      AxeOneHand	= 7,
      AxeTwoHand	= 8,
      MarksmanBow	= 9,
      MarksmanCrossbow	= 10,
      MarksmanThrown	= 11,
      Arrow		= 12,
      Bolt		= 13,
      Length
    }

  enum Flags : uint
    {
      Magical	= 0x01,
      Silver	= 0x02
    }

  align(1) struct WPDTstruct
  {
    float weight;
    int value;
    Type type;
    short health;
    float speed, reach;
    short enchant; // Enchantment points
    ubyte[2] chop, slash, thrust; // Min and max
    Flags flags;

    static assert(WPDTstruct.sizeof == 32);
  }

  WPDTstruct data;

  mixin LoadT!();

  MeshIndex model;
  IconIndex icon;
  Enchantment* enchant;
  Script* script;

  void load()
    {with(esFile){
      model = getMesh();
      name = getHNOString("FNAM");
      readHNExact(&data, data.sizeof, "WPDT");
      script = getHNOPtr!(Script)("SCRI", scripts);
      icon = getOIcon();
      enchant = getHNOPtr!(Enchantment)("ENAM", enchants);

      makeProto();

      proto.setFloat("speed", data.speed);
      proto.setFloat("reach", data.reach);

      proto.setInt("health", data.health);
    }}
}
ListID!(Weapon) weapons;
