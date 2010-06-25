#ifndef _ESM_ALCH_H
#define _ESM_ALCH_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM {

/*
 * Alchemy item (potions)
 */

struct Potion
{
  struct ALDTstruct
  {
    float weight;
    int value;
    int autoCalc;
  };
  ALDTstruct data;

  std::string name, model, icon, script;
  EffectList effects;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    icon = esm.getHNOString("TEXT"); // not ITEX here for some reason
    script = esm.getHNOString("SCRI");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "ALDT", 12);
    effects.load(esm);
  }
};
}
#endif
