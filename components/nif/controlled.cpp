#include "controlled.hpp"

#include "data.hpp"

namespace Nif
{

    void NiSourceTexture::read(NIFStream* nif)
    {
        Named::read(nif);

        external = nif->getChar() != 0;
        bool internal = false;
        if (external)
            filename = nif->getString();
        else
        {
            if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 3))
                internal = nif->getChar();
            if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 0))
                filename = nif->getString(); // Original file path of the internal texture
        }
        if (nif->getVersion() <= NIFStream::generateVersion(10, 0, 1, 3))
        {
            if (!external && internal)
                data.read(nif);
        }
        else
        {
            data.read(nif);
        }

        pixel = nif->getUInt();
        mipmap = nif->getUInt();
        alpha = nif->getUInt();

        // Renderer hints, typically of no use for us
        /* bool mIsStatic = */ nif->getChar();
        if (nif->getVersion() >= NIFStream::generateVersion(10, 1, 0, 103))
            /* bool mDirectRendering = */ nif->getBoolean();
        if (nif->getVersion() >= NIFStream::generateVersion(20, 2, 0, 4))
            /* bool mPersistRenderData = */ nif->getBoolean();
    }

    void NiSourceTexture::post(Reader& nif)
    {
        Named::post(nif);
        data.post(nif);
    }

    void BSShaderTextureSet::read(NIFStream* nif)
    {
        nif->getSizedStrings(textures, nif->getUInt());
    }

    void NiParticleModifier::read(NIFStream* nif)
    {
        mNext.read(nif);
        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            mController.read(nif);
    }

    void NiParticleModifier::post(Reader& nif)
    {
        mNext.post(nif);
        mController.post(nif);
    }

    void NiParticleGrowFade::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);
        nif->read(mGrowTime);
        nif->read(mFadeTime);
    }

    void NiParticleColorModifier::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        mData.read(nif);
    }

    void NiParticleColorModifier::post(Reader& nif)
    {
        NiParticleModifier::post(nif);

        mData.post(nif);
    }

    void NiGravity::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        if (nif->getVersion() >= NIFStream::generateVersion(3, 3, 0, 13))
            nif->read(mDecay);
        nif->read(mForce);
        mType = static_cast<ForceType>(nif->get<uint32_t>());
        nif->read(mPosition);
        nif->read(mDirection);
    }

    void NiParticleCollider::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        nif->read(mBounceFactor);
        if (nif->getVersion() >= NIFStream::generateVersion(4, 2, 0, 2))
        {
            nif->read(mSpawnOnCollision);
            nif->read(mDieOnCollision);
        }
    }

    void NiPlanarCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        nif->read(mExtents);
        nif->read(mPosition);
        nif->read(mXVector);
        nif->read(mYVector);
        nif->read(mPlaneNormal);
        nif->read(mPlaneDistance);
    }

    void NiSphericalCollider::read(NIFStream* nif)
    {
        NiParticleCollider::read(nif);

        nif->read(mRadius);
        nif->read(mCenter);
    }

    void NiParticleRotation::read(NIFStream* nif)
    {
        NiParticleModifier::read(nif);

        nif->read(mRandomInitialAxis);
        nif->read(mInitialAxis);
        nif->read(mRotationSpeed);
    }
}
