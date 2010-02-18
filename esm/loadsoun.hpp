#ifndef _ESM_SOUN_H
#define _ESM_SOUN_H

#include "esm_reader.hpp"

struct SOUNstruct
{
  unsigned char volume, minRange, maxRange;
};

struct Sound
{
  SOUNstruct data;
  std::string sound;

  void load(ESMReader &esm)
    {
      sound = esm.getHNString("FNAM");
      esm.getHNT(data, "DATA", 3);
    }
};
#endif
