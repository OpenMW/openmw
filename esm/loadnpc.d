/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadnpc.d) is part of the OpenMW package.

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

module esm.loadnpc;

import esm.imports;

import esm.loadcont;
import esm.loadrace;
import esm.loadclas;
import esm.loadfact;
import esm.loadbody;

/*
 * NPC definition
 */

struct NPC
{
  // Services
  enum Services : int
    {
      // Buys
      Weapon		= 0x00001,
      Armor		= 0x00002,
      Clothing		= 0x00004,
      Books		= 0x00008,
      Ingredients	= 0x00010,
      Picks		= 0x00020,
      Probes		= 0x00040,
      Lights		= 0x00080,
      Apparatus		= 0x00100,
      RepairItem	= 0x00200,
      Misc		= 0x00400,

      // Services
      Spells		= 0x00800,
      MagicItems	= 0x01000,
      Potions		= 0x02000,
      Training		= 0x04000, // What skills?
      Spellmaking	= 0x08000,
      Enchanting	= 0x10000,
      Repair		= 0x20000
    }

  enum Flags
    {
      Female	= 0x0001,
      Essential = 0x0002,
      Respawn	= 0x0004,
      Autocalc	= 0x0008,
      Skeleton	= 0x0400, // Skeleton blood effect (white)
      Metal	= 0x0800  // Metal blood effect (golden?)
    }

  align(1) struct NPDTstruct52
  {
    short level;
    byte strength, intelligence, willpower, agility,
      speed, endurance, personality, luck;
    byte skills[27];
    //byte reputation; // Total confusion!
    short health, mana, fatigue;
    byte disposition;
    byte reputation; // Was "factionID", but that makes no sense.
    byte rank, unknown, u2;
    int gold;
  }
  align(1) struct NPDTstruct12
  {
    short level;
    byte disposition, reputation, rank,
      unknown1, unknown2, unknown3;
    int gold; // ?? Not sure
  }

  align(1) struct AIDTstruct
  {
    // These are probabilities
    byte hello, u1, fight, flee, alarm, u2, u3, u4;
    // The u's might be the skills that this NPC can train you in
    Services services;

    static assert(AIDTstruct.sizeof == 12);
  }

  static assert(NPDTstruct52.sizeof==52);
  static assert(NPDTstruct12.sizeof==12);

  union
  {
    NPDTstruct52 npdt52;
    NPDTstruct12 npdt12; // Use this if npdt52.gold == -10
  }

  Flags flags;

  InventoryList inventory;
  SpellList spells;

  AIDTstruct AI;
  bool hasAI;

  LoadState state;

  char[] name, id;

  MeshIndex model;
  Race* race;
  Class* cls;
  Faction* faction;
  Script* script;
  BodyPart* hair, head;

  void load()
  {with(esFile){
    npdt52.gold = -10;

    model = getOMesh();
    name = getHNOString("FNAM");

    race = getHNPtr!(Race)("RNAM", races);
    cls = getHNPtr!(Class)("CNAM", classes);
    faction = getHNPtr!(Faction)("ANAM", factions);
    head = getHNPtr!(BodyPart)("BNAM", bodyParts);
    hair = getHNPtr!(BodyPart)("KNAM", bodyParts);

    script = getHNOPtr!(Script)("SCRI", scripts);

    getSubNameIs("NPDT");
    getSubHeader();
    if(getSubSize() == 52) readExact(&npdt52, npdt52.sizeof);
    else if(getSubSize() == 12) readExact(&npdt12, npdt12.sizeof);
    else fail("NPC_NPDT must be 12 or 52 bytes long");

    flags = cast(Flags) getHNInt("FLAG");

    inventory.load();
    spells.load();

    if(isNextSub("AIDT"))
      {
	readHExact(&AI, AI.sizeof);
	hasAI = true;
      }
      else hasAI = false;

    skipRecord();
  }}
}
ListID!(NPC) npcs;
