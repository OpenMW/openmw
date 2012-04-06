#include "loadsndg.hpp"

namespace ESM
{

void SoundGenerator::load(ESMReader &esm)
{
  esm.getHNT(type, "DATA", 4);

  creature = esm.getHNOString("CNAM");
  sound = esm.getHNOString("SNAM");
}
void SoundGenerator::save(ESMWriter &esm)
{
    esm.writeHNT("DATA", type, 4);
    esm.writeHNOString("CNAM", creature);
    esm.writeHNOString("SNAM", sound);
}

}
