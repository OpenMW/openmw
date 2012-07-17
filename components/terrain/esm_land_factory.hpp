#ifndef TERRAIN_ESM_LAND_FACTORY_H
#define TERRAIN_ESM_LAND_FACTORY_H

#include "land_factory.hpp"

namespace ESMS
{
  struct ESMStore;
}

namespace ESM
{
  class ESMReader;
}

namespace Terrain
{
  /*
    Land factory that loads data from ESM files.
  */
  class ESMLandFactory
  {
    ESMS::ESMStore &store;
    ESM::ESMReader &reader;

  public:
    // Initialize the land factory. Note that refrences to the given
    // store and reader are stored in the class, so the given objects
    // must be valid for a long as you plan to use this factory.
    ESMLandFactory(ESMS::ESMStore &st, ESM::ESMReader &rd)
      : store(st), reader(rd) {}

    // True if this factory has any data for the given grid cell.
    bool has(int x, int y);
  };
}
#endif
