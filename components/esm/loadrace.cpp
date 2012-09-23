#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Race::load(ESMReader &esm)
{
    mName = esm.getHNString("FNAM");
    esm.getHNT(mData, "RADT", 140);
    mPowers.load(esm);
    mDescription = esm.getHNOString("DESC");
}
void Race::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("RADT", mData, 140);
    mPowers.save(esm);
    esm.writeHNOString("DESC", mDescription);
}

}
