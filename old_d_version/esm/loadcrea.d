/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadcrea.d) is part of the OpenMW package.

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

module esm.loadcrea;
import esm.imports;

import esm.loadcont;

/*
 * Creature definition
 *
 * Container structs are in loadcont.d
 */

struct Creature
{
  // Default is 0x48?
  enum Flags
    {
      Biped		= 0x001,
      Respawn		= 0x002,
      Weapon		= 0x004, // Has weapon and shield
      None		= 0x008, // ??
      Swims		= 0x010,
      Flies		= 0x020, // Don't know what happens if several
      Walks		= 0x040, // of these are set
      Essential		= 0x080,
      Skeleton		= 0x400, // Does not have normal blood
      Metal		= 0x800  // Has 'golden' blood
    }

  enum Type : int
    {
      Creature	= 0,
      Deadra	= 1,
      Undead	= 2,
      Humanoid	= 3
    }

  align(1) struct NPDTstruct
  {
    Type type;
    // For creatures we obviously have to use ints, not shorts and
    // bytes like we use for NPCs.... this file format just makes so
    // much sense! (Still, _much_ easier to decode than the NIFs.)
    int level;
    int strength, intelligence, willpower, agility, speed, endurance,
      personality, luck, health, mana, fatigue; // Stats
    int soul; // The creatures soul value (used with soul gems.)
    int combat, magic, stealth; // Don't know yet.
    int attack[6]; // AttackMin1, AttackMax1, ditto2, ditto3
    int gold;

    static assert(NPDTstruct.sizeof == 96);
  }

  NPDTstruct data;

  Flags flags;

  mixin LoadT;

  MeshIndex model;

  // Base creature. Any missing data must be taken from here, I quess.
  Creature *original;
  Script *script;

  float scale;

  InventoryList inventory;

  void load()
    {with(esFile){
      model = getMesh();
      original = getHNOPtr!(Creature)("CNAM", creatures);
      name = getHNOString("FNAM");
      script = getHNOPtr!(Script)("SCRI", scripts);

      readHNExact(&data, data.sizeof, "NPDT");

      flags = cast(Flags)getHNInt("FLAG");
      scale = getHNOFloat("XSCL", 1.0);

      inventory.load();

      // AIDT - data (12 bytes, unknown)
      // AI_W - wander (14 bytes, i don't understand it)
      //    short distance
      //    byte duration
      //    byte timeOfDay
      //    byte idle[10]
      //
      // Rest is optional:
      // AI_T - travel?
      // AI_F - follow?
      // AI_E - escort?
      // AI_A - activate?

      //*
      skipRecord();
      //*/

      makeProto();

      proto.setInt("level", data.level);
      proto.setInt("gold", data.gold);

      proto.setInt("baseStrength", data.strength);
      proto.setInt("baseIntelligence", data.intelligence);
      proto.setInt("baseWillpower", data.willpower);
      proto.setInt("baseAgility", data.agility);
      proto.setInt("baseSpeed", data.speed);
      proto.setInt("baseEndurance", data.endurance);
      proto.setInt("basePersonality", data.personality);
      proto.setInt("baseLuck", data.luck);

      proto.setInt("baseMaxHealth", data.health);
      proto.setInt("baseMaxMana", data.mana);
      proto.setInt("baseMaxFatigue", data.fatigue);

      proto.setInt("combat", data.combat);
      proto.setInt("magic", data.magic);
      proto.setInt("stealth", data.stealth);
      proto.setInt("soul", data.soul);

      proto.setInt("attackMin1", data.attack[0]);
      proto.setInt("attackMax1", data.attack[1]);
      proto.setInt("attackMin2", data.attack[2]);
      proto.setInt("attackMax2", data.attack[3]);
      proto.setInt("attackMin3", data.attack[4]);
      proto.setInt("attackMax3", data.attack[5]);
    }}
 
}
ListID!(Creature) creatures;
