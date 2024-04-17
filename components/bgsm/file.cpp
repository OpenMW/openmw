#include "file.hpp"

#include "stream.hpp"

namespace Bgsm
{
    void MaterialFile::read(BGSMStream& stream)
    {
        stream.read(mVersion);
        stream.read(mClamp);
        stream.read(mUVOffset);
        stream.read(mUVScale);
        stream.read(mTransparency);
        stream.read(mAlphaBlend);
        stream.read(mSourceBlendMode);
        stream.read(mDestinationBlendMode);
        stream.read(mAlphaTestThreshold);
        stream.read(mAlphaTest);
        stream.read(mDepthWrite);
        stream.read(mSSR);
        stream.read(mWetnessControlSSR);
        stream.read(mDecal);
        stream.read(mTwoSided);
        stream.read(mDecalNoFade);
        stream.read(mNonOccluder);
        stream.read(mRefraction);
        stream.read(mRefractionFalloff);
        stream.read(mRefractionPower);
        if (mVersion < 10)
        {
            stream.read(mEnvMap);
            stream.read(mEnvMapMaskScale);
        }
        else
        {
            stream.read(mDepthBias);
        }
        stream.read(mGrayscaleToPaletteColor);
        if (mVersion >= 6)
        {
            stream.read(mMaskWrites);
        }
    }

    void BGSMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);
    }

    void BGEMFile::read(BGSMStream& stream)
    {
        MaterialFile::read(stream);
    }
}
