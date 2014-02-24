#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

void Enchantment::load(ESMReader &esm)
{
    esm.getHNT(mData, "ENDT", 16);
    mEffects.load(esm);
}

void Enchantment::save(ESMWriter &esm) const
{
    esm.writeHNT("ENDT", mData, 16);
    mEffects.save(esm);
}

}
