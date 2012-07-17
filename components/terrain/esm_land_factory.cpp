#include "esm_land_factory.hpp"

// The first one already includes the others implicitly, but it
// doesn't hurt to be explicit.
#include "../esm_store/store.hpp"
#include "../esm/esm_reader.hpp"
#include "../esm/loadland.hpp"

using namespace Terrain;

bool ESMLandFactory::has(int x, int y)
{
  return store.landscapes.has(x,y);
}
