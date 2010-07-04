#ifndef _ESM_GLOB_H
#define _ESM_GLOB_H

#include "esm_reader.hpp"

namespace ESM {

/*
 * Global script variables
 */

struct Global
{
  unsigned value;
  VarType type;

  void load(ESMReader &esm)
  {
    VarType t;
    std::string tmp = esm.getHNString("FNAM");
    if(tmp == "s") t = VT_Short;
    else if(tmp == "l") t = VT_Int;
    else if(tmp == "f") t = VT_Float;
    else esm.fail("Illegal global variable type " + tmp);
    type = t;

    // Note: Both floats and longs are represented as floats.
    esm.getHNT(value, "FLTV");
  }
};
}
#endif
