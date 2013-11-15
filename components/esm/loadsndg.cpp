#include "loadsndg.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int SoundGenerator::sRecordId = REC_SNDG;

void SoundGenerator::load(ESMReader &esm)
{
  esm.getHNT(mType, "DATA", 4);

  mCreature = esm.getHNOString("CNAM");
  mSound = esm.getHNOString("SNAM");
}
void SoundGenerator::save(ESMWriter &esm) const
{
    esm.writeHNT("DATA", mType, 4);
    esm.writeHNOCString("CNAM", mCreature);
    esm.writeHNOCString("SNAM", mSound);
}

}
