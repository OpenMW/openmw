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

    enum class ForceType : uint32_t
    {
        Wind = 0, // Fixed direction
        Point = 1, // Fixed origin
    };

    struct NiGravity : public NiParticleModifier
    {
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

    struct NiPSysAgeDeathModifier : NiPSysModifier
    {
        bool mSpawnOnDeath;
        NiPSysSpawnModifierPtr mSpawnModifier;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBoundUpdateModifier : public NiPSysModifier
    {
        uint16_t mUpdateSkip;

        void read(NIFStream* nif) override;
    };

    struct NiPSysDragModifier : public NiPSysModifier
    {
        NiAVObjectPtr mDragObject;
        osg::Vec3f mDragAxis;
        float mPercentage;
        float mRange;
        float mRangeFalloff;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysGravityModifier : public NiPSysModifier
    {
        NiAVObjectPtr mGravityObject;
        osg::Vec3f mGravityAxis;
        float mDecay;
        float mStrength;
        ForceType mForceType;
        float mTurbulence;
        float mTurbulenceScale;
        bool mWorldAligned;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysRotationModifier : public NiPSysModifier
    {
        float mRotationSpeed;
        float mRotationSpeedVariation;
        float mRotationAngle;
        float mRotationAngleVariation;
        bool mRandomRotSpeedSign;
        bool mRandomAxis;
        osg::Vec3f mAxis;

        void read(NIFStream* nif) override;
    };

    struct NiPSysSpawnModifier : NiPSysModifier
    {
        uint16_t mNumSpawnGenerations;
        float mPercentageSpawned;
        uint16_t mMinNumToSpawn;
        uint16_t mMaxNumToSpawn;
        float mSpawnSpeedVariation;
        float mSpawnDirVariation;
        float mLifespan;
        float mLifespanVariation;

        void read(NIFStream* nif) override;
    };

    struct BSPSysInheritVelocityModifier : public NiPSysModifier
    {
        NiAVObjectPtr mInheritObject;
        float mInheritChance;
        float mVelocityMult;
        float mVelcoityVariation;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSPSysLODModifier : NiPSysModifier
    {
        float mLODStartDistance;
        float mLODEndDistance;
        float mEndEmitScale;
        float mEndSize;

        void read(NIFStream* nif) override;
    };

    struct BSPSysRecycleBoundModifier : NiPSysModifier
    {
        osg::Vec3f mBoundOffset;
        osg::Vec3f mBoundExtents;
        NiAVObjectPtr mBoundObject;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSPSysScaleModifier : public NiPSysModifier
    {
        std::vector<float> mScales;

        void read(NIFStream* nif) override;
    };

    struct BSPSysSimpleColorModifier : NiPSysModifier
    {
        float mFadeInPercent;
        float mFadeOutPercent;
        float mColor1EndPercent;
        float mColor1StartPercent;
        float mColor2EndPercent;
        float mColor2StartPercent;
        std::vector<osg::Vec4f> mColors;

        void read(NIFStream* nif) override;
    };

    struct BSPSysStripUpdateModifier : NiPSysModifier
    {
        float mUpdateDeltaTime;

        void read(NIFStream* nif) override;
    };

    struct BSPSysSubTexModifier : public NiPSysModifier
    {
        float mStartFrame;
        float mStartFrameFudge;
        float mEndFrame;
        float mLoopStartFrame;
        float mLoopStartFrameFudge;
        float mFrameCount;
        float mFrameCountFudge;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct NiPSysEmitter : public NiPSysModifier
    {
        float mSpeed;
        float mSpeedVariation;
        float mDeclination;
        float mDeclinationVariation;
        float mPlanarAngle;
        float mPlanarAngleVariation;
        osg::Vec4f mInitialColor;
        float mInitialRadius;
        float mRadiusVariation;
        float mLifespan;
        float mLifespanVariation;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct NiPSysVolumeEmitter : public NiPSysEmitter
    {
        NiAVObjectPtr mEmitterObject;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBoxEmitter : public NiPSysVolumeEmitter
    {
        float mWidth;
        float mHeight;
        float mDepth;

        void read(NIFStream* nif) override;
    };

    struct NiPSysCylinderEmitter : public NiPSysVolumeEmitter
    {
        float mRadius;
        float mHeight;

        void read(NIFStream* nif) override;
    };

    struct NiPSysMeshEmitter : public NiPSysEmitter
    {
        NiAVObjectList mEmitterMeshes;
        uint32_t mInitialVelocityType;
        uint32_t mEmissionType;
        osg::Vec3f mEmissionAxis;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysSphereEmitter : public NiPSysVolumeEmitter
    {
        float mRadius;

        void read(NIFStream* nif) override;
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

    struct NiPSysCollider : Record
    {
        float mBounce;
        bool mCollideSpawn;
        bool mCollideDie;
        NiPSysSpawnModifierPtr mSpawnModifier;
        NiPSysColliderManagerPtr mParent;
        NiPSysColliderPtr mNextCollider;
        NiAVObjectPtr mColliderObject;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysColliderManager : NiPSysModifier
    {
        NiPSysColliderPtr mCollider;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysPlanarCollider : NiPSysCollider
    {
        float mWidth;
        float mHeight;
        osg::Vec3f mXAxis;
        osg::Vec3f mYAxis;

        void read(NIFStream* nif) override;
    };

    struct NiPSysSphericalCollider : NiPSysCollider
    {
        float mRadius;

        void read(NIFStream* nif) override;
    };

}
#endif
