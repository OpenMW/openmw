#ifndef _ESM_SKIL_H
#define _ESM_SKIL_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM {

/*
 * Skill information
 *
 */

struct Skill
{
  struct SKDTstruct
  {
    int attribute;     // see defs.hpp
    int specialization;// 0 - Combat, 1 - Magic, 2 - Stealth
    float useValue[4]; // How much skill improves through use. Meaning
		       // of each field depends on what skill this
		       // is. We should document this better later.
  }; // Total size: 24 bytes
  SKDTstruct data;

  // Skill index. Skils don't have an id ("NAME") like most records,
  // they only have a numerical index that matches one of the
  // hard-coded skills in the game.
  int index;

  std::string description;

    enum SkillEnum
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
    };
  static const std::string sSkillNameIds[Length];

  void load(ESMReader &esm)
    {
      esm.getHNT(index, "INDX");
      esm.getHNT(data, "SKDT", 24);
      description = esm.getHNOString("DESC");
    }
};
}
#endif
