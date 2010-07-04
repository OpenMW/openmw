#ifndef _ESM_ACTI_H
#define _ESM_ACTI_H

#include "esm_reader.hpp"

namespace ESM {

struct Activator
{
  std::string name, script, model;

  void load(ESMReader &esm)
  {
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    script = esm.getHNOString("SCRI");
  }
};
}
#endif
