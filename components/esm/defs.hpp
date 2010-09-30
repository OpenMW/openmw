#ifndef _ESM_DEFS_H
#define _ESM_DEFS_H

#include "esm_reader.hpp"

namespace ESM {

// Pixel color value. Standard four-byte rr,gg,bb,aa format.
typedef int32_t Color;

enum VarType
  {
    VT_Unknown,
    VT_None,
    VT_Short,
    VT_Int,
    VT_Long,
    VT_Float,
    VT_String,
    VT_Ignored
  };

enum Specialization
  {
    SPC_Combat  = 0,
    SPC_Magic   = 1,
    SPC_Stealth = 2
  };

enum RangeType
  {
    RT_Self = 0,
    RT_Touch = 1,
    RT_Target = 2
  };

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

// Position and rotation
struct Position
{
  float pos[3];
  float rot[3];
};

struct ENAMstruct
{
  // Magical effect, hard-coded ID
  short effectID;

  // Which skills/attributes are affected (for restore/drain spells
  // etc.)
  signed char skill, attribute; // -1 if N/A

  // Other spell parameters
  int range; // 0 - self, 1 - touch, 2 - target (RangeType enum)
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
