#include "esm_land_factory.hpp"

// The first one already includes the others implicitly, but it
// doesn't hurt to be explicit.
#include "../esm_store/store.hpp"
#include "../esm/esm_reader.hpp"
#include "../esm/loadland.hpp"

using namespace Terrain;

static class ESMLandStream : public Mangle::Stream
{
public:
  ESMLandStream(ESM::Land *l, ESM::ESMReader &r)
  {
  }
};

bool ESMLandFactory::has(int x, int y)
{
  return store.landscapes.has(x,y);
}

LandDataPtr get(int x, int y, LandInfo &info)
{
  assert(has(x,y));

  // Set up the info
  info.grid = LGT_Quadratic;
  info.data = LDT_Float;

  const float SIZE = 8192; // CHECK

  info.xsize = SIZE;
  info.ysize = SIZE;
  info.numx = 65;
  info.numy = 65;
  info.xoffset = SIZE*x;
  info.yoffset = SIZE*y;

  // Get the Land struct from store
  ESM::Land* land = store.landscapes.find(x,y);
  assert(land->hasData);

  // Create a stream for the data and return it.
  LandDataPtr ptr(new ESMLandStream(land, reader));
  return ptr;
}
