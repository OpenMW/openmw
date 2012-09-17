#include "loadltex.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

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
