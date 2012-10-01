#include "loadltex.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void LandTexture::load(ESMReader &esm)
{
    esm.getHNT(mIndex, "INTV");
    mTexture = esm.getHNString("DATA");
}
void LandTexture::save(ESMWriter &esm)
{
    esm.writeHNT("INTV", mIndex);
    esm.writeHNCString("DATA", mTexture);
}

}
