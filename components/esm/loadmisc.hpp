#ifndef _ESM_MISC_H
#define _ESM_MISC_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Misc inventory items, basically things that have no use but can be
 * carried, bought and sold. It also includes keys.
 */

struct Misc
{
  struct MCDTstruct
  {
    float weight;
    int value;
    int isKey; // There are many keys in Morrowind.esm that has this
           // set to 0. TODO: Check what this field corresponds to
           // in the editor.
  };
  MCDTstruct data;

  std::string name, model, icon, script;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "MCDT", 12);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
  }
};
}
#endif
