#ifndef _ESM_SNDG_H
#define _ESM_SNDG_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Sound generator. This describes the sounds a creature make.
 */

struct SoundGenerator
{
  enum Type
    {
      LeftFoot  = 0,
      RightFoot = 1,
      SwimLeft  = 2,
      SwimRight = 3,
      Moan  = 4,
      Roar  = 5,
      Scream    = 6,
      Land  = 7
    };

  // Type 
  int type;

  std::string creature, sound;

  void load(ESMReader &esm)
  {
    esm.getHNT(type, "DATA", 4);

    creature = esm.getHNOString("CNAM");
    sound = esm.getHNOString("SNAM");
  }
};
}
#endif
