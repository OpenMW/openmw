#ifndef _ESM_CREC_H
#define _ESM_CREC_H

#include "esm_reader.hpp"

namespace ESM {

/* These two are only used in save games. They are not decoded yet.
 */

/// Changes a creature
struct LoadCREC
{
  void load(ESMReader &esm)
    {
      esm.skipRecord();
    }
};

/// Changes an item list / container
struct LoadCNTC
{
  void load(ESMReader &esm)
    {
      esm.skipRecord();
    }
};
}
#endif
