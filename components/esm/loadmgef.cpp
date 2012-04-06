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
void MagicEffect::save(ESMWriter &esm)
{
    esm.writeHNT("INDX", index);
    esm.writeHNT("MEDT", data, 36);

    esm.writeHNOString("ITEX", icon);
    esm.writeHNOString("PTEX", particle);
    esm.writeHNOString("BSND", boltSound);
    esm.writeHNOString("CSND", castSound);
    esm.writeHNOString("HSND", hitSound);
    esm.writeHNOString("ASND", areaSound);
    
    esm.writeHNOString("CVFX", casting);
    esm.writeHNOString("BVFX", bolt);
    esm.writeHNOString("HVFX", hit);
    esm.writeHNOString("AVFX", area);
    
    esm.writeHNOString("DESC", description);
}

}
