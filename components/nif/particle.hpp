
#ifndef OPENMW_COMPONENTS_NIF_PARTICLE_HPP
#define OPENMW_COMPONENTS_NIF_PARTICLE_HPP

#include "nifkey.hpp"
#include "niftypes.hpp" // Transformation
#include "recordptr.hpp"
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>
#include <components/settings/values.hpp>

namespace Nif
{
    struct NiParticles : NiGeometry
    {
    };

    struct NiParticlesData : public NiGeometryData
    {
        unsigned short numParticles{ 0 };

        int activeCount{ 0 };

        std::vector<float> particleRadii, sizes, mRotationAngles;
        std::vector<osg::Quat> mRotations;
        std::vector<osg::Vec3f> mRotationAxes;
        std::vector<osg::Vec4f> mSubtexOffsets;

        float mAspectRatio;
        unsigned short mAspectFlags;
        float mAspect2, mSpeed1, mSpeed2;

        void read(NIFStream* nif) override;
    };

    struct NiRotatingParticlesData : public NiParticlesData
    {
        void read(NIFStream* nif) override;
    };

    struct NiParticleInfo
    {
        osg::Vec3f mVelocity;
        osg::Vec3f mRotation;
        float mAge;
        float mLifeSpan;
        float mLastUpdate;
        unsigned short mSpawnGen;
        unsigned short mCode;

        void read(NIFStream* nif);
    };

    struct NiPSysData : public NiParticlesData
    {
        std::vector<NiParticleInfo> mParticleInfo;
        std::vector<float> mRotationSpeeds;
        unsigned short mParticlesAddedNum;
        unsigned short mParticlesBase;

        void read(NIFStream* nif) override;
    };

    struct NiParticleSystem : public NiParticles
    {
        BSVertexDesc mVertexDesc;
        unsigned short mFarBegin;
        unsigned short mFarEnd;
        unsigned short mNearBegin;
        unsigned short mNearEnd;
        NiPSysDataPtr mNiPSysData;

        bool mWorldSpace;
        std::vector<NiPSysModifierPtr> mModifiers;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysModifier : public Record
    {
        std::string mName;
        unsigned int mOrder;
        NiParticleSystemPtr mTarget;
        bool mActive;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysModifierCtlr : public NiSingleInterpController
    {
        std::string mModifierName;

        void read(NIFStream* nif) override;
    };

    struct NiPSysEmitterCtlr : public NiPSysModifierCtlr
    {
        NiInterpolatorPtr mVisibilityInterpolator;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysAgeDeathModifier : public NiPSysModifier
    {
        bool mSpawnOnDeath;
        NiPSysSpawnModifierPtr mSpawnModifier;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysSpawnModifier : public NiPSysModifier
    {
        unsigned short mNumSpawnGens;
        float mPercentSpawned;
        unsigned short mMinSpawnNum, mMaxSpawnNum;
        float mSpawnSpeedVariation;
        float mSpawnDirVariation;
        float mLifeSpan;
        float mLifeSpanVariation;

        void read(NIFStream* nif) override;
    };

    struct BSPSysLODModifier : public NiPSysModifier
    {
        float mBeginDist;
        float mEndDist;
        float mEndEmitScale;
        float mEndSize;

        void read(NIFStream* nif) override;
    };

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

    struct NiPSysVolumeEmitter : public NiPSysEmitter
    {
        NiNodePtr mEmitterObject;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysCylinderEmitter : public NiPSysVolumeEmitter
    {
        float mRadius;
        float mHeight;

        void read(NIFStream* nif) override;
    };

    struct BSPSysSimpleColorModifier : public NiPSysModifier
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

    struct NiPSysRotationModifier : public NiPSysModifier
    {
        float mRotationSpeed;
        float mRotationSpeedVariation;
        float mRotationAngle;
        float mRotationAngleVariation;

        bool mRandRotSpeedSign;
        bool mRandAxis;
        osg::Vec3f mAxis;

        void read(NIFStream* nif) override;
    };

    struct BSPSysScaleModifier : public NiPSysModifier
    {
        std::vector<float> mScales;

        void read(NIFStream* nif) override;
    };

    struct NiPSysGravityModifier : public NiPSysModifier
    {
        NamedPtr mGravityObject;
        osg::Vec3f mGravityAxis;
        float mDecay;
        float mStrength;
        unsigned int mForceType;
        float mTurbulence;
        float mTurbulenceScale;

        bool mWorldAligned;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBoundUpdateModifier : public NiPSysModifier
    {
        unsigned short mUpdateSkip;

        void read(NIFStream* nif) override;
    };

    struct NiPSysModifierActiveCtlr : public NiPSysModifierCtlr
    {
        NiVisDataPtr mNiVisData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysMeshEmitter : public NiPSysEmitter
    {
        std::vector<NamedPtr> mEmitterMeshes;
        unsigned int mInitialVelocityType;
        unsigned int mEmissionType;
        osg::Vec3f mEmissionAxis;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct BSPSysInheritVelocityModifier : public NiPSysModifier
    {
        NamedPtr mInheritObject;
        float mInheritChance;
        float mVelocityMult;
        float mVelcoityVariation;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysBombModifier : public NiPSysModifier
    {
        NiNodePtr mBombObj;
        osg::Vec3f mBombAxis;
        float mDecay;
        float mDeltaV;

        unsigned int mDecayType;
        unsigned int mSymmetryType;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPSysDragModifier : public NiPSysModifier
    {
        NamedPtr mDragObj;
        osg::Vec3f mDragAxis;
        float mPercentage;
        float mRange;
        float mRangeFalloff;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };
}

#endif
