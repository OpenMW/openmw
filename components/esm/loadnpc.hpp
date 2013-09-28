#ifndef _ESM_NPC_H
#define _ESM_NPC_H

#include "esm_reader.hpp"
#include "loadcont.hpp"
#include "defs.hpp"
#include "aipackage.hpp"

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
      Weapon        = 0x00001,
      Armor         = 0x00002,
      Clothing      = 0x00004,
      Books         = 0x00008,
      Ingredients   = 0x00010,
      Picks         = 0x00020,
      Probes        = 0x00040,
      Lights        = 0x00080,
      Apparatus     = 0x00100,
      RepairItem    = 0x00200,
      Misc          = 0x00400,

      // Other services
      Spells        = 0x00800,
      MagicItems    = 0x01000,
      Potions       = 0x02000,
      Training      = 0x04000, // What skills?
      Spellmaking   = 0x08000,
      Enchanting    = 0x10000,
      Repair        = 0x20000
    };

  enum Flags
    {
      Female    = 0x0001,
      Essential = 0x0002,
      Respawn   = 0x0004,
      Autocalc  = 0x0008,
      Skeleton  = 0x0400, // Skeleton blood effect (white)
      Metal     = 0x0800  // Metal blood effect (golden?)
    };

#pragma pack(push)
#pragma pack(1)
  struct NPDTstruct52
  {
    short level;
    char strength, intelligence, willpower, agility,
      speed, endurance, personality, luck;
    char skills[27];
    char reputation;
    short health, mana, fatigue;
    char disposition, factionID, rank;
    char unknown;
    int gold;
  }; // 52 bytes

  struct NPDTstruct12
  {
    short level;
    char disposition, reputation, rank,
      unknown1, unknown2, unknown3;
    int gold; // ?? not certain
  }; // 12 bytes
#pragma pack(pop)

    struct Dest
    {
        Position    mPos;
        std::string mCellName;
    };

  NPDTstruct52 npdt52;
  NPDTstruct12 npdt12; // Use this if npdt52.gold == -10

  int flags;

  InventoryList inventory;
  SpellList spells;

    AIData mAiData;
    bool mHasAI;

    std::vector<Dest> mTransport;
    AIPackageList     mAiPackage;

    std::string name, model, race, cls, faction, script;

    // body parts
    std::string hair, head;

    std::string mId;

    // Implementation moved to load_impl.cpp
    void load(ESMReader &esm, const std::string& id);
};
}
#endif
