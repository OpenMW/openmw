#include "effect.hpp"

#include "node.hpp"
#include "texture.hpp"

namespace Nif
{

    void NiDynamicEffect::read(NIFStream* nif)
    {
        NiAVObject::read(nif);

        if (nif->getVersion() > NIFFile::VER_MW && nif->getVersion() < nif->generateVersion(10, 1, 0, 0))
            return;

        if (nif->getBethVersion() >= NIFFile::BethVersion::BETHVER_FO4)
            return;

        if (nif->getVersion() >= nif->generateVersion(10, 1, 0, 106))
            nif->read(mSwitchState);
        size_t numAffectedNodes = nif->get<uint32_t>();
        nif->skip(numAffectedNodes * 4);
    }

    void NiLight::read(NIFStream* nif)
    {
        NiDynamicEffect::read(nif);

        nif->read(mDimmer);
        nif->read(mAmbient);
        nif->read(mDiffuse);
        nif->read(mSpecular);
    }

    void NiPointLight::read(NIFStream* nif)
    {
        NiLight::read(nif);

        nif->read(mConstantAttenuation);
        nif->read(mLinearAttenuation);
        nif->read(mQuadraticAttenuation);
    }

    void NiSpotLight::read(NIFStream* nif)
    {
        NiPointLight::read(nif);

        nif->read(mOuterSpotAngle);
        if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 5))
            nif->read(mInnerSpotAngle);
        nif->read(mExponent);
    }

    void NiTextureEffect::read(NIFStream* nif)
    {
        NiDynamicEffect::read(nif);

        nif->read(mProjectionRotation);
        nif->read(mProjectionPosition);
        nif->read(mFilterMode);
        if (nif->getVersion() >= NIFStream::generateVersion(20, 5, 0, 4))
            nif->read(mMaxAnisotropy);
        nif->read(mClampMode);
        mTextureType = static_cast<TextureType>(nif->get<uint32_t>());
        mCoordGenType = static_cast<CoordGenType>(nif->get<uint32_t>());
        mTexture.read(nif);
        nif->read(mEnableClipPlane);
        mClipPlane = osg::Plane(nif->get<osg::Vec4f>());
        if (nif->getVersion() <= NIFStream::generateVersion(10, 2, 0, 0))
            nif->skip(4); // PS2-specific shorts
        if (nif->getVersion() <= NIFStream::generateVersion(4, 1, 0, 12))
            nif->skip(2); // Unknown short
    }

    void NiTextureEffect::post(Reader& nif)
    {
        NiDynamicEffect::post(nif);

        mTexture.post(nif);
    }

}
