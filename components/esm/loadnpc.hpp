#ifndef _ESM_NPC_H
#define _ESM_NPC_H

#include "esm_reader.hpp"
#include "loadcont.hpp"
#include "defs.hpp"

namespace ESM {

/*
 * NPC definition
 */

struct NPC
{
  // Services
  enum Services
    {
      // This merchant buys:
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

      // Other services
      Spells		= 0x00800,
      MagicItems	= 0x01000,
      Potions		= 0x02000,
      Training		= 0x04000, // What skills?
      Spellmaking	= 0x08000,
      Enchanting	= 0x10000,
      Repair		= 0x20000
    };

  enum Flags
    {
      Female	= 0x0001,
      Essential = 0x0002,
      Respawn	= 0x0004,
      Autocalc	= 0x0008,
      Skeleton	= 0x0400, // Skeleton blood effect (white)
      Metal	= 0x0800  // Metal blood effect (golden?)
    };

#pragma pack(push)
#pragma pack(1)
  struct NPDTstruct52
  {
    short level;
    char strength, intelligence, willpower, agility,
      speed, endurance, personality, luck;
    char skills[27];
    short health, mana, fatigue;
    char disposition;
    char reputation; // Was "factionID", but that makes no sense.
    char rank, unknown, u2;
    int gold;
  }; // 52 bytes

  struct NPDTstruct12
  {
    short level;
    char disposition, reputation, rank,
      unknown1, unknown2, unknown3;
    int gold; // ?? not certain
  }; // 12 bytes

  struct AIDTstruct
  {
    // These are probabilities
    char hello, u1, fight, flee, alarm, u2, u3, u4;
    // The last u's might be the skills that this NPC can train you
    // in?
    int services; // See the Services enum
  }; // 12 bytes

#pragma pack(pop)

  NPDTstruct52 npdt52;
  NPDTstruct12 npdt12; // Use this if npdt52.gold == -10

  int flags;

  InventoryList inventory;
  SpellList spells;

  AIDTstruct AI;
  bool hasAI;

  std::string name, model, race, cls, faction, script,
    hair, head; // body parts

    std::string mId;

  void load(ESMReader &esm, const std::string& id)
  {
    mId = id;

    npdt52.gold = -10;

    model = esm.getHNOString("MODL");
    name = esm.getHNOString("FNAM");

    race = esm.getHNString("RNAM");
    cls = esm.getHNString("CNAM");
    faction = esm.getHNString("ANAM");
    head = esm.getHNString("BNAM");
    hair = esm.getHNString("KNAM");

    script = esm.getHNOString("SCRI");

    esm.getSubNameIs("NPDT");
    esm.getSubHeader();
    if(esm.getSubSize() == 52) esm.getExact(&npdt52, 52);
    else if(esm.getSubSize() == 12) esm.getExact(&npdt12, 12);
    else esm.fail("NPC_NPDT must be 12 or 52 bytes long");

    esm.getHNT(flags, "FLAG");

    inventory.load(esm);
    spells.load(esm);

    if(esm.isNextSub("AIDT"))
      {
	esm.getHExact(&AI, sizeof(AI));
	hasAI = true;
      }
    else hasAI = false;

    esm.skipRecord();
  }
};
}
#endif
