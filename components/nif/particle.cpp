#include "particle.hpp"

#include "data.hpp"

namespace Nif
{

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
