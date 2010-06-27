#ifndef _ESM_CONT_H
#define _ESM_CONT_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Container definition
 */

struct ContItem
{
  int count;
  NAME32 item;
};

struct InventoryList
{
  std::vector<ContItem> list;

  void load(ESMReader &esm)
  {
    ContItem ci;
    while(esm.isNextSub("NPCO"))
      {
        esm.getHT(ci, 36);
        list.push_back(ci);
      }
  }	
};

struct Container
{
  enum Flags
    {
      Organic	= 1, // Objects cannot be placed in this container
      Respawn	= 2, // Respawns after 4 months
      Unknown	= 8
    };

  std::string name, model, script;

  float weight; // Not sure, might be max total weight allowed?
  int flags;
  InventoryList inventory;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(weight, "CNDT", 4);
    esm.getHNT(flags, "FLAG", 4);

    if(flags & 0xf4) esm.fail("Unknown flags");
    if(!(flags & 0x8)) esm.fail("Flag 8 not set");

    script = esm.getHNOString("SCRI");

    inventory.load(esm);
  }
};
}
#endif
