#include "loadsndg.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void SoundGenerator::load(ESMReader &esm)
{
  esm.getHNT(mType, "DATA", 4);

  mCreature = esm.getHNOString("CNAM");
  mSound = esm.getHNOString("SNAM");
}
void SoundGenerator::save(ESMWriter &esm)
{
    esm.writeHNT("DATA", mType, 4);
    esm.writeHNOCString("CNAM", mCreature);
    esm.writeHNOCString("SNAM", mSound);
}

}
