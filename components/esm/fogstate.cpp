#include "fogstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

void ESM::FogState::load (ESMReader &esm)
{
    esm.getHNOT(mBounds, "BOUN");
    esm.getHNOT(mNorthMarkerAngle, "ANGL");
    while (esm.isNextSub("FTEX"))
    {
        esm.getSubHeader();
        FogTexture tex;

        esm.getT(tex.mX);
        esm.getT(tex.mY);

        size_t imageSize = esm.getSubSize()-sizeof(int)*2;
        tex.mImageData.resize(imageSize);
        esm.getExact(&tex.mImageData[0], imageSize);
        mFogTextures.push_back(tex);
    }
}

void ESM::FogState::save (ESMWriter &esm, bool interiorCell) const
{
    if (interiorCell)
    {
        esm.writeHNT("BOUN", mBounds);
        esm.writeHNT("ANGL", mNorthMarkerAngle);
    }
    for (std::vector<FogTexture>::const_iterator it = mFogTextures.begin(); it != mFogTextures.end(); ++it)
    {
        esm.startSubRecord("FTEX");
        esm.writeT(it->mX);
        esm.writeT(it->mY);
        esm.write(&it->mImageData[0], it->mImageData.size());
        esm.endRecord("FTEX");
    }
}
