#include "texture.hpp"

#include "data.hpp"

namespace Nif
{

    void NiSourceTexture::read(NIFStream* nif)
    {
        NiTexture::read(nif);

        nif->read(mExternal);
        bool hasData = nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 4);
        if (!hasData && !mExternal)
            hasData = nif->get<uint8_t>() != 0;

        if (mExternal || nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
            nif->read(mFile);

        if (hasData)
            mData.read(nif);

        mPrefs.mPixelLayout = static_cast<PixelLayout>(nif->get<uint32_t>());
        mPrefs.mUseMipMaps = static_cast<MipMapFormat>(nif->get<uint32_t>());
        mPrefs.mAlphaFormat = static_cast<AlphaFormat>(nif->get<uint32_t>());

        nif->read(mIsStatic);
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 103))
        {
            nif->read(mDirectRendering);
            if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 4))
                nif->read(mPersistRenderData);
        }
    }

    void NiSourceTexture::post(Reader& nif)
    {
        NiTexture::post(nif);
        mData.post(nif);
    }

    void BSShaderTextureSet::read(NIFStream* nif)
    {
        nif->getSizedStrings(mTextures, nif->get<uint32_t>());
    }

}
