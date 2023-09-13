#ifndef OPENMW_COMPONENTS_NIF_CONTROLLER_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLER_HPP

#include "base.hpp"
#include "niftypes.hpp"
#include "property.hpp"

namespace Nif
{

    struct ControlledBlock
    {
        std::string mTargetName;
        NiInterpolatorPtr mInterpolator;
        NiTimeControllerPtr mController;
        NiBlendInterpolatorPtr mBlendInterpolator;
        uint16_t mBlendIndex;
        uint8_t mPriority;
        NiStringPalettePtr mStringPalette;
        uint32_t mNodeNameOffset;
        uint32_t mPropertyTypeOffset;
        uint32_t mControllerTypeOffset;
        uint32_t mControllerIdOffset;
        uint32_t mInterpolatorIdOffset;
        std::string mNodeName;
        std::string mPropertyType;
        std::string mControllerType;
        std::string mControllerId;
        std::string mInterpolatorId;

        void read(NIFStream* nif);
        void post(Reader& nif);
    };

    // Gamebryo KF root node record type (pre-10.0)
    struct NiSequence : public Record
    {
        std::string mName;
        std::string mAccumRootName;
        ExtraPtr mTextKeys;
        uint32_t mArrayGrowBy;
        std::vector<ControlledBlock> mControlledBlocks;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Gamebryo KF root node record type (10.0+)
    struct NiControllerSequence : public NiSequence
    {
        float mWeight{ 1.f };
        NiTimeController::ExtrapolationMode mExtrapolationMode{ NiTimeController::ExtrapolationMode::Constant };
        float mFrequency{ 1.f };
        float mPhase{ 1.f };
        float mStartTime, mStopTime;
        bool mPlayBackwards{ false };
        NiControllerManagerPtr mManager;
        NiStringPalettePtr mStringPalette;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Base class for controllers that use NiInterpolators to animate objects.
    struct NiInterpController : public NiTimeController
    {
        // Usually one of the flags.
        bool mManagerControlled{ false };

        void read(NIFStream* nif) override;
    };

    // Base class for controllers that use one NiInterpolator.
    struct NiSingleInterpController : public NiInterpController
    {
        NiInterpolatorPtr mInterpolator;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Base class for controllers that use a NiFloatInterpolator to animate their target.
    struct NiFloatInterpController : public NiSingleInterpController
    {
    };

    // Ditto for NiBoolInterpolator.
    struct NiBoolInterpController : public NiSingleInterpController
    {
    };

    // Ditto for NiPoint3Interpolator.
    struct NiPoint3InterpController : public NiSingleInterpController
    {
    };

    struct NiParticleSystemController : public NiTimeController
    {
        enum BSPArrayController
        {
            BSPArrayController_AtNode = 0x8,
            BSPArrayController_AtVertex = 0x10
        };

        struct Particle
        {
            osg::Vec3f velocity;
            float lifetime;
            float lifespan;
            float timestamp;
            unsigned short vertex;
        };

        float velocity;
        float velocityRandom;

        float verticalDir; // 0=up, pi/2=horizontal, pi=down
        float verticalAngle;
        float horizontalDir;
        float horizontalAngle;

        osg::Vec4f color;
        float size;
        float startTime;
        float stopTime;

        float emitRate;
        float lifetime;
        float lifetimeRandom;

        enum EmitFlags
        {
            EmitFlag_NoAutoAdjust = 0x1 // If this flag is set, we use the emitRate value. Otherwise,
                                        // we calculate an emit rate so that the maximum number of particles
                                        // in the system (numParticles) is never exceeded.
        };
        int emitFlags;

        osg::Vec3f offsetRandom;

        NiAVObjectPtr emitter;

        int numParticles;
        int activeCount;
        std::vector<Particle> particles;

        NiParticleModifierPtr affectors;
        NiParticleModifierPtr colliders;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;

        bool noAutoAdjust() const { return emitFlags & EmitFlag_NoAutoAdjust; }
        bool emitAtVertex() const { return mFlags & BSPArrayController_AtVertex; }
    };
    using NiBSPArrayController = NiParticleSystemController;

    struct NiMaterialColorController : public NiPoint3InterpController
    {
        enum class TargetColor
        {
            Ambient = 0,
            Diffuse = 1,
            Specular = 2,
            Emissive = 3,
        };

        NiPosDataPtr mData;
        TargetColor mTargetColor;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiPathController : public NiTimeController
    {
        enum Flags
        {
            Flag_CVDataNeedsUpdate = 0x01,
            Flag_OpenCurve = 0x02,
            Flag_AllowFlip = 0x04,
            Flag_Bank = 0x08,
            Flag_ConstVelocity = 0x10,
            Flag_Follow = 0x20,
            Flag_FlipFollowAxis = 0x40,
        };

        uint16_t mPathFlags;
        int32_t mBankDirection;
        float mMaxBankAngle;
        float mSmoothing;
        uint16_t mFollowAxis;
        NiPosDataPtr mPathData;
        NiFloatDataPtr mPercentData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiLookAtController : public NiTimeController
    {
        enum Flags
        {
            Flag_Flip = 0x1,
            Flag_LookYAxis = 0x2,
            Flag_LookZAxis = 0x4,
        };

        uint16_t mLookAtFlags{ 0 };
        NiAVObjectPtr mLookAt;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiUVController : public NiTimeController
    {
        NiUVDataPtr mData;
        uint16_t mUvSet;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiKeyframeController : public NiSingleInterpController
    {
        NiKeyframeDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiMultiTargetTransformController : public NiInterpController
    {
        NiAVObjectList mExtraTargets;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiAlphaController : public NiFloatInterpController
    {
        NiFloatDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiRollController : public NiSingleInterpController
    {
        NiFloatDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiGeomMorpherController : public NiInterpController
    {
        bool mUpdateNormals{ false };
        bool mAlwaysActive{ false };
        NiMorphDataPtr mData;
        NiInterpolatorList mInterpolators;
        std::vector<float> mWeights;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiVisController : public NiBoolInterpController
    {
        NiVisDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiFlipController : public NiFloatInterpController
    {
        NiTexturingProperty::TextureType mTexSlot;
        float mDelta; // Time between two flips. delta = (start_time - stop_time) / num_sources
        NiSourceTextureList mSources;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct NiTextureTransformController : public NiFloatInterpController
    {
        bool mShaderMap;
        NiTexturingProperty::TextureType mTexSlot;
        uint32_t mTransformMember;
        NiFloatDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkBlendController : public NiTimeController
    {
        void read(NIFStream* nif) override;
    };

    struct BSEffectShaderPropertyFloatController : public NiFloatInterpController
    {
        uint32_t mControlledVariable;

        void read(NIFStream* nif) override;
    };

    struct BSEffectShaderPropertyColorController : public NiPoint3InterpController
    {
        uint32_t mControlledColor;

        void read(NIFStream* nif) override;
    };

    struct NiControllerManager : public NiTimeController
    {
        bool mCumulative;
        NiControllerSequenceList mSequences;
        NiDefaultAVObjectPalettePtr mObjectPalette;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Abstract
    struct NiInterpolator : public Record
    {
    };

    template<class T, class DataPtr>
    struct TypedNiInterpolator : public NiInterpolator
    {
        T mDefaultValue;
        DataPtr mData;

        void read(NIFStream* nif) override
        {
            nif->read(mDefaultValue);
            mData.read(nif);
        }

        void post(Reader& nif) override
        {
            mData.post(nif);
        }
    };

    using NiPoint3Interpolator = TypedNiInterpolator<osg::Vec3f, NiPosDataPtr>;
    using NiBoolInterpolator = TypedNiInterpolator<bool, NiBoolDataPtr>;
    using NiFloatInterpolator = TypedNiInterpolator<float, NiFloatDataPtr>;
    using NiTransformInterpolator = TypedNiInterpolator<NiQuatTransform, NiKeyframeDataPtr>;
    using NiColorInterpolator = TypedNiInterpolator<osg::Vec4f, NiColorDataPtr>;

    // Abstract
    struct NiBlendInterpolator : public NiInterpolator
    {
        enum Flags
        {
            Flag_ManagerControlled = 0x1,
            Flag_OnlyUseHighestWeight = 0x2,
        };

        struct Item
        {
            NiInterpolatorPtr mInterpolator;
            float mWeight, mNormalizedWeight;
            int32_t mPriority;
            float mEaseSpinner;

            void read(NIFStream* nif);
            void post(Reader& nif);
        };

        uint8_t mFlags{ 0 };
        uint16_t mArrayGrowBy{ 0 };
        float mWeightThreshold;
        uint16_t mInterpCount;
        uint16_t mSingleIndex;
        int32_t mHighPriority, mNextHighPriority;
        float mSingleTime;
        float mHighWeightsSum, mNextHighWeightsSum;
        float mHighEaseSpinner;
        std::vector<Item> mItems;
        NiInterpolatorPtr mSingleInterpolator;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    template <typename T>
    struct TypedNiBlendInterpolator : public NiBlendInterpolator
    {
        T mValue;

        void read(NIFStream* nif) override
        {
            NiBlendInterpolator::read(nif);

            nif->read(mValue);
        }
    };

    template <>
    struct TypedNiBlendInterpolator<NiQuatTransform> : public NiBlendInterpolator
    {
        NiQuatTransform mValue;

        void read(NIFStream* nif) override
        {
            NiBlendInterpolator::read(nif);

            if (nif->getVersion() <= NIFStream::generateVersion(10, 1, 0, 109))
                nif->read(mValue);
        }
    };

    using NiBlendBoolInterpolator = TypedNiBlendInterpolator<uint8_t>;
    using NiBlendFloatInterpolator = TypedNiBlendInterpolator<float>;
    using NiBlendPoint3Interpolator = TypedNiBlendInterpolator<osg::Vec3f>;
    using NiBlendTransformInterpolator = TypedNiBlendInterpolator<NiQuatTransform>;

}
#endif
