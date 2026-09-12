#include "fogstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    void FogState::load(ESMReader& esm)
    {
        if (esm.isNextSub("BOUN"))
            esm.getHT(mBounds.mMinX, mBounds.mMinY, mBounds.mMaxX, mBounds.mMaxY);
        esm.getHNOT(mNorthMarkerAngle, "ANGL");
        if (!esm.getHNOT("CNTR", mCenterX, mCenterY))
        {
            mCenterX = (mBounds.mMinX + mBounds.mMaxX) / 2;
            mCenterY = (mBounds.mMinY + mBounds.mMaxY) / 2;
        }
        const FormatVersion dataFormat = esm.getFormatVersion();
        while (esm.isNextSub("FTEX"))
        {
            esm.getSubHeader();
            FogTexture tex;

            esm.getT(tex.mX);
            esm.getT(tex.mY);

            const std::size_t imageSize = esm.getSubSize() - sizeof(int32_t) * 2;
            tex.mImageData.resize(imageSize);
            esm.getExact(tex.mImageData.data(), imageSize);

            tex.mLegacyTgaImageData = dataFormat <= MaxOldFogOfWarFormatVersion;

            mFogTextures.push_back(std::move(tex));
        }
    }

    void FogState::save(ESMWriter& esm, bool interiorCell) const
    {
        if (interiorCell)
        {
            esm.writeHNT("BOUN", mBounds);
            esm.writeHNT("ANGL", mNorthMarkerAngle);
            esm.startSubRecord("CNTR");
            esm.writeT(mCenterX);
            esm.writeT(mCenterY);
            esm.endRecord("CNTR");
        }
        for (const FogTexture& texture : mFogTextures)
        {
            esm.startSubRecord("FTEX");
            esm.writeT(texture.mX);
            esm.writeT(texture.mY);
            esm.write(texture.mImageData.data(), texture.mImageData.size());
            esm.endRecord("FTEX");
        }
    }

}
