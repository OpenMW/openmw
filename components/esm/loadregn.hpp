#ifndef _ESM_REGN_H
#define _ESM_REGN_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Region data
 */

struct Region
{
#pragma pack(push)
#pragma pack(1)
  struct WEATstruct
  {
    // I guess these are probabilities
    char clear, cloudy, foggy, overcast, rain, thunder, ash,
      blight,
    // Unknown weather, probably snow and something. Only
    // present in file version 1.3.
      a,b;
  }; // 10 bytes

  // Reference to a sound that is played randomly in this region
  struct SoundRef
  {
    NAME32 sound;
    char chance;
  }; // 33 bytes
#pragma pack(pop)

  WEATstruct data;
  int mapColor; // RGBA

  // sleepList refers to a eveled list of creatures you can meet if
  // you sleep outside in this region.
  std::string name, sleepList;

  std::vector<SoundRef> soundList;

  void load(ESMReader &esm)
  {
    name = esm.getHNString("FNAM");

    if(esm.getVer() == VER_12)
      esm.getHNExact(&data, sizeof(data)-2, "WEAT");
    else if(esm.getVer() == VER_13)
      esm.getHNExact(&data, sizeof(data), "WEAT");
    else esm.fail("Don't know what to do in this version");

    sleepList = esm.getHNOString("BNAM");

    esm.getHNT(mapColor, "CNAM");

    while(esm.hasMoreSubs())
      {
        SoundRef sr;
        esm.getHNT(sr, "SNAM", 33);
        soundList.push_back(sr);
      }
  }
};
}
#endif
