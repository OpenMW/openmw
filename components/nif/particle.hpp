#ifndef OPENMW_COMPONENTS_NIF_PARTICLE_HPP
#define OPENMW_COMPONENTS_NIF_PARTICLE_HPP

#include "base.hpp"
#include "controller.hpp"
#include "data.hpp"
#include "node.hpp"

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

    struct NiParticlesData : NiGeometryData
    {
        uint16_t mNumParticles{ 0 };
        uint16_t mActiveCount;

        std::vector<float> mRadii;
        std::vector<float> mSizes;
        std::vector<osg::Quat> mRotations;
        std::vector<float> mRotationAngles;
        std::vector<osg::Vec3f> mRotationAxes;

        bool mHasTextureIndices{ false };
        std::vector<osg::Vec4f> mSubtextureOffsets;
        float mAspectRatio{ 1.f };
        uint16_t mAspectFlags{ 0 };
        float mAspectRatio2;
        float mAspectSpeed, mAspectSpeed2;

        void read(NIFStream* nif) override;
    };

    struct NiRotatingParticlesData : NiParticlesData
    {
        void read(NIFStream* nif) override;
    };

    struct NiParticleSystem : NiParticles
    {
        osg::BoundingSpheref mBoundingSphere;
        std::array<float, 6> mBoundMinMax;
        BSVertexDesc mVertDesc;
        std::array<uint16_t, 4> mNearFar;
        bool mWorldSpace{ true };
        NiPSysModifierList mModifiers;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysData : NiParticlesData
    {
        std::vector<NiParticleInfo> mParticles;
        std::vector<float> mRotationSpeeds;
        uint16_t mNumAddedParticles;
        uint16_t mAddedParticlesBase;

        void read(NIFStream* nif) override;
    };

    struct BSStripPSysData : NiPSysData
    {
        uint16_t mMaxPointCount;
        float mStartCapSize;
        float mEndCapSize;
        bool mDoZPrepass;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct NiPSysModifier : Record
    {
        enum class NiPSysModifierOrder : uint32_t
        {
            KillOldParticles = 0,
            BSLOD = 1,
            Emitter = 1000,
            Spawn = 2000,
            BSStripUpdateFO3 = 2500,
            General = 3000,
            Force = 4000,
            Collider = 5000,
            PosUpdate = 6000,
            PostPosUpdate = 6500,
            WorldshiftPartspawn = 6600,
            BoundUpdate = 7000,
            BSStripUpdateSK = 8000,
        };

        std::string mName;
        NiPSysModifierOrder mOrder;
        NiParticleSystemPtr mTarget;
        bool mActive;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Abstract
    struct NiPSysModifierCtlr : NiSingleInterpController
    {
        std::string mModifierName;

        void read(NIFStream* nif) override;
    };

    template <class DataPtr>
    struct TypedNiPSysModifierCtlr : NiPSysModifierCtlr
    {
        DataPtr mData;

        void read(NIFStream* nif) override
        {
            NiPSysModifierCtlr::read(nif);

            if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 103))
                mData.read(nif);
        }

        void post(Reader& nif) override
        {
            NiPSysModifierCtlr::post(nif);

            mData.post(nif);
        }
    };

    using NiPSysModifierBoolCtlr = TypedNiPSysModifierCtlr<NiVisDataPtr>;
    using NiPSysModifierFloatCtlr = TypedNiPSysModifierCtlr<NiFloatDataPtr>;

    struct NiPSysEmitterCtlr : NiPSysModifierCtlr
    {
        NiPSysEmitterCtlrDataPtr mData;
        NiInterpolatorPtr mVisInterpolator;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysEmitterCtlrData : Record
    {
        FloatKeyMapPtr mFloatKeyList;
        BoolKeyMapPtr mVisKeyList;

        void read(NIFStream* nif) override;
    };

}
#endif
