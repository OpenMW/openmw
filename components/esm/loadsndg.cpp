#include "loadsndg.hpp"

namespace ESM
{

void SoundGenerator::load(ESMReader &esm)
{
  esm.getHNT(type, "DATA", 4);

  creature = esm.getHNOString("CNAM");
  sound = esm.getHNOString("SNAM");
}

}
