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

    esm.writeHNOCString("ITEX", icon);
    esm.writeHNOCString("PTEX", particle);
    esm.writeHNOCString("BSND", boltSound);
    esm.writeHNOCString("CSND", castSound);
    esm.writeHNOCString("HSND", hitSound);
    esm.writeHNOCString("ASND", areaSound);
    
    esm.writeHNOCString("CVFX", casting);
    esm.writeHNOCString("BVFX", bolt);
    esm.writeHNOCString("HVFX", hit);
    esm.writeHNOCString("AVFX", area);
    
    esm.writeHNOString("DESC", description);
}

}
