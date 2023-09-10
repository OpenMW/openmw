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

        mDimmer = nif->getFloat();
        mAmbient = nif->getVector3();
        mDiffuse = nif->getVector3();
        mSpecular = nif->getVector3();
    }

    void NiPointLight::read(NIFStream* nif)
    {
        NiLight::read(nif);

        mConstantAttenuation = nif->getFloat();
        mLinearAttenuation = nif->getFloat();
        mQuadraticAttenuation = nif->getFloat();
    }

    void NiSpotLight::read(NIFStream* nif)
    {
        NiPointLight::read(nif);

        mCutoff = nif->getFloat();
        mExponent = nif->getFloat();
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
