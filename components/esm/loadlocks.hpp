#ifndef _ESM_LOCKS_H
#define _ESM_LOCKS_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * This file covers lockpicks (LOCK), probes (PROB) and armor repair
 * items (REPA). These have nearly identical data structures.
 */

struct Tool
{
  struct Data
  {
    float weight;
    int value;

    float quality; // And when I say nearly identical structure, I
    int uses;      // mean perfectly identical except that these two
                   // variables are swaped for repair items. Don't ask
                   // me why.
  }; // Size = 16

  Data data;
  std::string name, model, icon, script;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");

    esm.getSubName();
    NAME n = esm.retSubName();
    // The data name varies, RIDT for repair items, LKDT for lock
    // picks, PBDT for probes

    esm.getHT(data, 16);

    if(n == "RIDT")
      {
        // Swap t.data.quality and t.data.uses for repair items (sigh)
        float tmp = *((float*)&data.uses);
        data.uses = *((int*)&data.quality);
        data.quality = tmp;
      }

    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
  }
};
}
#endif
