#ifndef OPENMW_COMPONENTS_NIF_PARTICLE_HPP
#define OPENMW_COMPONENTS_NIF_PARTICLE_HPP

#include "base.hpp"
#include "controller.hpp"
#include "data.hpp"
#include "node.hpp"

namespace Nif
{

    struct NiParticleModifier : Record
    {
        NiParticleModifierPtr mNext;
        NiTimeControllerPtr mController;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiParticleGrowFade : NiParticleModifier
    {
        float mGrowTime;
        float mFadeTime;

        void read(NIFStream* nif) override;
    };

    struct NiParticleColorModifier : NiParticleModifier
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

    enum class DecayType : uint32_t
    {
        None = 0, // f(Distance) = 1.0
        Linear = 1, // f(Distance) = (Range - Distance) / Range
        Exponential = 2, // f(Distance) = exp(-Distance / Range)
    };

    enum class SymmetryType : uint32_t
    {
        Spherical = 0,
        Cylindrical = 1, // Perpendicular to direction axis
        Planar = 2, // Parallel to direction axis
    };

    struct NiGravity : NiParticleModifier
    {
        float mDecay{ 0.f };
        float mForce;
        ForceType mType;
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;

        void read(NIFStream* nif) override;
    };

    struct NiParticleBomb : NiParticleModifier
    {
        float mRange;
        float mDuration;
        float mStrength;
        float mStartTime;
        DecayType mDecayType;
        SymmetryType mSymmetryType;
        osg::Vec3f mPosition;
        osg::Vec3f mDirection;

        void read(NIFStream* nif);
    };

    struct NiParticleCollider : NiParticleModifier
    {
        float mBounceFactor;
        bool mSpawnOnCollision{ false };
        bool mDieOnCollision{ false };

        void read(NIFStream* nif) override;
    };

    // NiPinaColada
    struct NiPlanarCollider : NiParticleCollider
    {
        osg::Vec2f mExtents;
        osg::Vec3f mPosition;
        osg::Vec3f mXVector, mYVector;
        osg::Vec3f mPlaneNormal;
        float mPlaneDistance;

        void read(NIFStream* nif) override;
    };

    struct NiSphericalCollider : NiParticleCollider
    {
        float mRadius;
        osg::Vec3f mCenter;

        void read(NIFStream* nif) override;
    };

    struct NiParticleRotation : NiParticleModifier
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

    struct BSMasterParticleSystem : NiNode
    {
        uint16_t mMaxEmitters;
        NiAVObjectList mParticleSystems;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
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

    struct NiMeshPSysData : NiPSysData
    {
        uint32_t mDefaultPoolSize;
        bool mFillPoolsOnLoad;
        std::vector<uint32_t> mGenerations;
        NiAVObjectPtr mParticleMeshes;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
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

    struct NiPSysBombModifier : NiPSysModifier
    {
        NiAVObjectPtr mBombObject;
        osg::Vec3f mBombAxis;
        float mRange;
        float mStrength;
        DecayType mDecayType;
        SymmetryType mSymmetryType;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBoundUpdateModifier : NiPSysModifier
    {
        uint16_t mUpdateSkip;

        void read(NIFStream* nif) override;
    };

    struct NiPSysColorModifier : NiPSysModifier
    {
        NiColorDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysDragModifier : NiPSysModifier
    {
        NiAVObjectPtr mDragObject;
        osg::Vec3f mDragAxis;
        float mPercentage;
        float mRange;
        float mRangeFalloff;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysGravityModifier : NiPSysModifier
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

    struct NiPSysGrowFadeModifier : NiPSysModifier
    {
        float mGrowTime;
        uint16_t mGrowGeneration;
        float mFadeTime;
        uint16_t mFadeGeneration;
        float mBaseScale;

        void read(NIFStream* nif) override;
    };

    struct NiPSysMeshUpdateModifier : NiPSysModifier
    {
        NiAVObjectList mMeshes;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysRotationModifier : NiPSysModifier
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

    struct BSParentVelocityModifier : NiPSysModifier
    {
        float mDamping;

        void read(NIFStream* nif) override;
    };

    struct BSPSysHavokUpdateModifier : NiPSysMeshUpdateModifier
    {
        NiPSysModifierPtr mModifier;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSPSysInheritVelocityModifier : NiPSysModifier
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

    struct BSPSysScaleModifier : NiPSysModifier
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

    struct BSPSysSubTexModifier : NiPSysModifier
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

    struct BSWindModifier : NiPSysModifier
    {
        float mStrength;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct NiPSysEmitter : NiPSysModifier
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
    struct NiPSysVolumeEmitter : NiPSysEmitter
    {
        NiAVObjectPtr mEmitterObject;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBoxEmitter : NiPSysVolumeEmitter
    {
        float mWidth;
        float mHeight;
        float mDepth;

        void read(NIFStream* nif) override;
    };

    struct NiPSysCylinderEmitter : NiPSysVolumeEmitter
    {
        float mRadius;
        float mHeight;

        void read(NIFStream* nif) override;
    };

    struct NiPSysMeshEmitter : NiPSysEmitter
    {
        NiAVObjectList mEmitterMeshes;
        uint32_t mInitialVelocityType;
        uint32_t mEmissionType;
        osg::Vec3f mEmissionAxis;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysSphereEmitter : NiPSysVolumeEmitter
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

    struct BSPSysMultiTargetEmitterCtlr : NiPSysEmitterCtlr
    {
        uint16_t mMaxEmitters;
        BSMasterParticleSystemPtr mMasterPSys;

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
