#ifndef _ESM_BOOK_H
#define _ESM_BOOK_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Books, magic scrolls, notes and so on
 */

struct Book
{
  struct BKDTstruct
  {
    float weight;
    int value, isScroll, skillID, enchant;
  };

  BKDTstruct data;
  std::string name, model, icon, script, enchant, text;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "BKDT", 20);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
    text = esm.getHNOString("TEXT");
    enchant = esm.getHNOString("ENAM");
  }
};
}
#endif
