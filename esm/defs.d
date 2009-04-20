/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (defs.d) is part of the OpenMW package.

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

module esm.defs;

public import std.string;
public import monster.util.string;
import monster.monster;

/*
 *  Types and definitions related to parsing esm and esp files
 */

alias char[4] NAME;
alias char[32] NAME32;
alias char[256] NAME256;

union Color
{
  align(1) struct
  {
    ubyte red, green, blue, alpha;
  }

  ubyte[4] array;
  uint value;

  char[] toString() { return format("RGBA:%s", array); }
}
static assert(Color.sizeof==4);

// State of a record struct
enum LoadState
  {
    Unloaded,	// This record is not loaded, it has just been
	        // referenced.
    Loaded,   	// This record has been loaded by the current file
    Previous  	// The record has been loaded by a previous file

    // Finalized - might be the case for some record types, but I
    // don't know if this actual state value would be used for
    // anything.
  }

enum VarType { Unknown, None, Short, Int, Long, Float, String, Ignored }

enum SpellSchool : int
  {
    Alteration = 0,
    Conjuration = 1,
    Destruction = 2,
    Illusion = 3,
    Mysticism = 4,
    Restoration = 5,
    Length
  }

enum Attribute : int
  {
    Strength = 0,
    Intelligence = 1,
    Willpower = 2,
    Agility = 3,
    Speed = 4,
    Endurance = 5,
    Personality = 6,
    Luck = 7,
    Length
  }

enum SkillEnum : int
  {
    Block = 0,
    Armorer = 1,
    MediumArmor = 2,
    HeavyArmor = 3,
    BluntWeapon = 4,
    LongBlade = 5,
    Axe = 6,
    Spear = 7,
    Athletics = 8,
    Enchant = 9,
    Destruction = 10,
    Alteration = 11,
    Illusion = 12,
    Conjuration = 13,
    Mysticism = 14,
    Restoration = 15,
    Alchemy = 16,
    Unarmored = 17,
    Security = 18,
    Sneak = 19,
    Acrobatics = 20,
    LightArmor = 21,
    ShortBlade = 22,
    Marksman = 23,
    Mercantile = 24,
    Speechcraft = 25,
    HandToHand = 26,
    Length
  }

// Shared between SPEL (Spells), ALCH (Potions) and ENCH (Item
// enchantments) records
align(1) struct ENAMstruct
{
  // Magical effect
  short effectID; // ID of magic effect

  // Which skills/attributes are affected (for restore/drain spells etc.)
  byte skill, attribute; // -1 if N/A

  // Other spell parameters
  int range; // 0 - self, 1 - touch, 2 - target
  int area, duration, magnMin, magnMax;

  static assert(ENAMstruct.sizeof==24);
}

// Common stuff for all the load* structs
template LoadTT(T)
{
  LoadState state;
  char[] name, id;

  MonsterObject *proto;
  static MonsterClass mc;

  void makeProto(char[] clsName = null)
    {
      // Use the template type name as the Monster class name if none
      // is specified.
      if(clsName == "")
        {
          clsName = typeid(T).toString;

          // Remove the module name
          int i = clsName.rfind('.');
          if(i != -1)
            clsName = clsName[i+1..$];
        }

      // Set up a prototype object
      if(mc is null)
        mc = vm.load(clsName);
      proto = mc.createObject();

      proto.setString8("id", id);
      proto.setString8("name", name);

      static if(is(typeof(data.weight) == float))
        {
          proto.setFloat("weight", data.weight);
          proto.setInt("value", data.value);
        }

      static if(is(typeof(data.enchant)==int))
        proto.setInt("enchant", data.enchant);
    }
}

template LoadT() { mixin LoadTT!(typeof(*this)); }
