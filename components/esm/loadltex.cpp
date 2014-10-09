#include "loadltex.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int LandTexture::sRecordId = REC_LTEX;

void LandTexture::load(ESMReader &esm)
{
    esm.getHNT(mIndex, "INTV");
    mTexture = esm.getHNString("DATA");
}
void LandTexture::save(ESMWriter &esm) const
{
    esm.writeHNT("INTV", mIndex);
    esm.writeHNCString("DATA", mTexture);
}

void LandTexture::blank()
{
    mTexture.clear();
    mIndex = -1;
}

}
