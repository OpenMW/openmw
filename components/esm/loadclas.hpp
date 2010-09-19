#ifndef _ESM_CLAS_H
#define _ESM_CLAS_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Character class definitions
 */

// These flags tells us which items should be auto-calculated for this
// class
struct Class
{
  enum AutoCalc
    {
      Weapon		= 0x00001,
      Armor		= 0x00002,
      Clothing	 	= 0x00004,
      Books		= 0x00008,
      Ingredient	= 0x00010,
      Lockpick		= 0x00020,
      Probe		= 0x00040,
      Lights		= 0x00080,
      Apparatus		= 0x00100,
      Repair		= 0x00200,
      Misc		= 0x00400,
      Spells		= 0x00800,
      MagicItems	= 0x01000,
      Potions		= 0x02000,
      Training		= 0x04000,
      Spellmaking	= 0x08000,
      Enchanting	= 0x10000,
      RepairItem	= 0x20000
    };

  enum Specialization
    {
      Combat = 0,
      Magic = 1,
      Stealth = 2
    };

  struct CLDTstruct
  {
    int attribute[2];   // Attributes that get class bonus
    int specialization; // 0 = Combat, 1 = Magic, 2 = Stealth
    int skills[5][2];   // Minor and major skills.
    int isPlayable;     // 0x0001 - Playable class

    // I have no idea how to autocalculate these items...
    int calc;
  }; // 60 bytes

  std::string name, description;
  CLDTstruct data;

  void load(ESMReader &esm)
    {
      name = esm.getHNString("FNAM");
      esm.getHNT(data, "CLDT", 60);

      if(data.isPlayable > 1)
	esm.fail("Unknown bool value");

      description = esm.getHNOString("DESC");
    }
};
}
#endif
