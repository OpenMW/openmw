#include "loadmgef.hpp"

namespace ESM
{

void MagicEffect::load(ESMReader &esm)
{
  esm.getHNT(index, "INDX");

  esm.getHNT(data, "MEDT", 36);
  icon = esm.getHNOString("ITEX");
  particle = esm.getHNOString("PTEX");

  boltSound = esm.getHNOString("BSND");
  castSound = esm.getHNOString("CSND");
  hitSound = esm.getHNOString("HSND");
  areaSound = esm.getHNOString("ASND");

  casting = esm.getHNOString("CVFX");
  bolt = esm.getHNOString("BVFX");
  hit = esm.getHNOString("HVFX");
  area = esm.getHNOString("AVFX");

  description = esm.getHNOString("DESC");
}

}
