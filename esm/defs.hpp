#ifndef _ESM_DEFS_H
#define _ESM_DEFS_H

#include "esm_reader.hpp"

namespace ESM {

/** A list of references to spells and spell effects. This is shared
    between the records BSGN, NPC and RACE.
*/
struct SpellList
{
  std::vector<std::string> list;

  void load(ESMReader &esm)
  {
    while(esm.isNextSub("NPCS"))
      list.push_back(esm.getHString());
  }
};

/** Defines a spell effect. Shared between SPEL (Spells), ALCH
    (Potions) and ENCH (Item enchantments) records
*/
#pragma pack(push)
#pragma pack(1)
struct ENAMstruct
{
  // Magical effect, hard-coded ID
  short effectID;

  // Which skills/attributes are affected (for restore/drain spells
  // etc.)
  char skill, attribute; // -1 if N/A

  // Other spell parameters
  int range; // 0 - self, 1 - touch, 2 - target
  int area, duration, magnMin, magnMax;

  // Struct size should be 24 bytes
};
#pragma pack(pop)

struct EffectList
{
  std::vector<ENAMstruct> list;

  void load(ESMReader &esm)
  {
    ENAMstruct s;    
    while(esm.isNextSub("ENAM"))
      {
        esm.getHT(s, 24);
        list.push_back(s);
      }
  }
};

}
#endif
