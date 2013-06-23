#ifndef _ESM_RACE_H
#define _ESM_RACE_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Race definition
 */

struct Race
{
  struct SkillBonus
  {
    int skill; // SkillEnum
    int bonus;
  };

  struct MaleFemale
  {
    int male, female;
  };

  struct MaleFemaleF
  {
    float male, female;
  };

  enum Flags
    {
      Playable  = 0x01,
      Beast = 0x02
    };

  struct RADTstruct
  {
    // List of skills that get a bonus
    SkillBonus bonus[7];

    // Attribute values for male/female
    MaleFemale strength, intelligence, willpower, agility,
      speed, endurance, personality, luck;

    // The actual eye level height (in game units) is (probably) given
    // as 'height' times 128. This has not been tested yet.
    MaleFemaleF height, weight;

    int flags; // 0x1 - playable, 0x2 - beast race

    // Size = 140 bytes
  };

  RADTstruct data;

  std::string name, description;
  SpellList powers;

  void load(ESMReader &esm)
  {
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "RADT", 140);
    powers.load(esm);
    description = esm.getHNOString("DESC");
  }
};
}
#endif
