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

  void load(ESMReader &esm)
    {
      esm.getHNT(data, "SKDT", 24);
      description = esm.getHNOString("DESC");
    }
};
}
#endif
