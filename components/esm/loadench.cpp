#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Enchantment::load(ESMReader &esm)
{
    esm.getHNT(mData, "ENDT", 16);
    mEffects.load(esm);
}

void Enchantment::save(ESMWriter &esm)
{
    esm.writeHNT("ENDT", mData, 16);
    mEffects.save(esm);
}

}
