#ifndef _ESM_ENCH_H
#define _ESM_ENCH_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Enchantments
 */

struct Enchantment
{
  enum Type
    {
      CastOnce		= 0,
      WhenStrikes	= 1,
      WhenUsed		= 2,
      ConstantEffect	= 3
    };

  struct ENDTstruct
  {
    int type;
    int cost;
    int charge;
    int autocalc; // Guessing this is 1 if we are supposed to auto
		  // calculate
  };

  ENDTstruct data;
  EffectList effects;

  void load(ESMReader &esm)
  {
    esm.getHNT(data, "ENDT", 16);
    effects.load(esm);
  }
};
}
#endif
