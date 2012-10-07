#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Spell::load(ESMReader &esm)
{
    mName = esm.getHNOString("FNAM");
    esm.getHNT(mData, "SPDT", 12);
    mEffects.load(esm);
}

void Spell::save(ESMWriter &esm)
{
    esm.writeHNOCString("FNAM", mName);
    esm.writeHNT("SPDT", mData, 12);
    mEffects.save(esm);
}

}
