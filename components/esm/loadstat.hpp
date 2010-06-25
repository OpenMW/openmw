#ifndef _ESM_STAT_H
#define _ESM_STAT_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Definition of static object.
 *
 * A stat record is basically just a reference to a nif file. Some
 * esps seem to contain copies of the STAT entries from the esms, and
 * the esms themselves contain several identical entries. Perhaps all
 * statics referenced in a file is also put in the file? Since we are
 * only reading files it doesn't much matter to us, but it would if we
 * were writing our own ESM/ESPs. You can check some files later when
 * you decode the CELL blocks, if you want to test this hypothesis.
 */

struct Static
{
  std::string model;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
  }
};
}
#endif
