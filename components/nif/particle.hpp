#ifndef OPENMW_COMPONENTS_NIF_PARTICLE_HPP
#define OPENMW_COMPONENTS_NIF_PARTICLE_HPP

#include "base.hpp"

namespace Nif
{

    struct NiParticleModifier : public Record
    {
        NiParticleModifierPtr mNext;
        NiTimeControllerPtr mController;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiParticleGrowFade : public NiParticleModifier
    {
        float mGrowTime;
        float mFadeTime;

        void read(NIFStream* nif) override;
    };

    struct NiParticleColorModifier : public NiParticleModifier
    {
        NiColorDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiGravity : public NiParticleModifier
    {
        enum class ForceType : uint32_t
        {
            Wind = 0, // Fixed direction
            Point = 1, // Fixed origin
        };

        float mDecay{ 0.f };
        float mForce;
        ForceType mType;
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;

        void read(NIFStream* nif) override;
    };

    struct NiParticleCollider : public NiParticleModifier
    {
        float mBounceFactor;
        bool mSpawnOnCollision{ false };
        bool mDieOnCollision{ false };

        void read(NIFStream* nif) override;
    };

    // NiPinaColada
    struct NiPlanarCollider : public NiParticleCollider
    {
        osg::Vec2f mExtents;
        osg::Vec3f mPosition;
        osg::Vec3f mXVector, mYVector;
        osg::Vec3f mPlaneNormal;
        float mPlaneDistance;

        void read(NIFStream* nif) override;
    };

    struct NiSphericalCollider : public NiParticleCollider
    {
        float mRadius;
        osg::Vec3f mCenter;

        void read(NIFStream* nif) override;
    };

    struct NiParticleRotation : public NiParticleModifier
    {
        uint8_t mRandomInitialAxis;
        osg::Vec3f mInitialAxis;
        float mRotationSpeed;

        void read(NIFStream* nif) override;
    };

}
#endif
