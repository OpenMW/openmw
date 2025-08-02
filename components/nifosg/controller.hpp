#ifndef COMPONENTS_NIFOSG_CONTROLLER_H
#define COMPONENTS_NIFOSG_CONTROLLER_H

#include <set>
#include <type_traits>

#include <osg/Texture2D>

#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>
#include <components/nif/nifkey.hpp>
#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/statesetupdater.hpp>

namespace osg
{
    class Material;
    class MatrixTransform;
}

namespace osgParticle
{
    class ParticleProcessor;
}

namespace SceneUtil
{
    class MorphGeometry;
}

namespace NifOsg
{

    class MatrixTransform;

    // interpolation of keyframes
    template <typename MapT>
    class ValueInterpolator
    {
        typename MapT::MapType::const_iterator retrieveKey(float time) const
        {
            // retrieve the current position in the map, optimized for the most common case
            // where time moves linearly along the keyframe track
            if (mLastHighKey != mKeys->mKeys.end())
            {
                if (time > mLastHighKey->first)
                {
                    // try if we're there by incrementing one
                    ++mLastLowKey;
                    ++mLastHighKey;
                }
                if (mLastHighKey != mKeys->mKeys.end() && time >= mLastLowKey->first && time <= mLastHighKey->first)
                    return mLastHighKey;
            }

            return std::lower_bound(mKeys->mKeys.begin(), mKeys->mKeys.end(), time,
                [](const typename MapT::MapType::value_type& key, float t) { return key.first < t; });
        }

    public:
        using ValueT = typename MapT::ValueType;

        ValueInterpolator() = default;

        template <class T,
            typename = std::enable_if_t<
                std::conjunction_v<std::disjunction<std::is_same<ValueT, float>, std::is_same<ValueT, osg::Vec3f>,
                                       std::is_same<ValueT, bool>, std::is_same<ValueT, osg::Vec4f>>,
                    std::is_same<decltype(T::mDefaultValue), ValueT>>,
                T>>
        ValueInterpolator(const T* interpolator)
            : mDefaultVal(interpolator->mDefaultValue)
        {
            if (interpolator->mData.empty())
                return;
            mKeys = interpolator->mData->mKeyList;
            if (mKeys)
            {
                mLastLowKey = mKeys->mKeys.end();
                mLastHighKey = mKeys->mKeys.end();
            }
        }

        ValueInterpolator(std::shared_ptr<const MapT> keys, ValueT defaultVal = ValueT())
            : mKeys(keys)
            , mDefaultVal(defaultVal)
        {
            if (keys)
            {
                mLastLowKey = mKeys->mKeys.end();
                mLastHighKey = mKeys->mKeys.end();
            }
        }

        ValueT interpKey(float time) const
        {
            if (empty())
                return mDefaultVal;

            const typename MapT::MapType& keys = mKeys->mKeys;

            if (time <= keys.front().first)
                return keys.front().second.mValue;

            typename MapT::MapType::const_iterator it = retrieveKey(time);

            // now do the actual interpolation
            if (it != keys.end())
            {
                // cache for next time
                mLastHighKey = it;
                mLastLowKey = --it;

                const float highTime = mLastHighKey->first;
                const float lowTime = mLastLowKey->first;
                if (highTime == lowTime)
                    return mLastLowKey->second.mValue;

                const float a = (time - lowTime) / (highTime - lowTime);

                return interpolate(mLastLowKey->second, mLastHighKey->second, a, mKeys->mInterpolationType);
            }

            return keys.back().second.mValue;
        }

        bool empty() const { return !mKeys || mKeys->mKeys.empty(); }

    private:
        template <typename ValueType>
        ValueType interpolate(
            const Nif::KeyT<ValueType>& a, const Nif::KeyT<ValueType>& b, float fraction, unsigned int type) const
        {
            switch (type)
            {
                case Nif::InterpolationType_Constant:
                    return fraction > 0.5f ? b.mValue : a.mValue;
                case Nif::InterpolationType_Quadratic:
                case Nif::InterpolationType_TCB:
                {
                    // Using a cubic Hermite spline.
                    // b1(t) = 2t^3  - 3t^2 + 1
                    // b2(t) = -2t^3 + 3t^2
                    // b3(t) = t^3 - 2t^2 + t
                    // b4(t) = t^3 - t^2
                    // f(t) = a.mValue * b1(t) + b.mValue * b2(t) + a.mOutTan * b3(t) + b.mInTan * b4(t)
                    const float t = fraction;
                    const float t2 = t * t;
                    const float t3 = t2 * t;
                    const float b1 = 2.f * t3 - 3.f * t2 + 1;
                    const float b2 = -2.f * t3 + 3.f * t2;
                    const float b3 = t3 - 2.f * t2 + t;
                    const float b4 = t3 - t2;
                    return a.mValue * b1 + b.mValue * b2 + a.mOutTan * b3 + b.mInTan * b4;
                }
                default:
                    return a.mValue + ((b.mValue - a.mValue) * fraction);
            }
        }
        osg::Quat interpolate(
            const Nif::KeyT<osg::Quat>& a, const Nif::KeyT<osg::Quat>& b, float fraction, unsigned int type) const
        {
            switch (type)
            {
                case Nif::InterpolationType_Constant:
                    return fraction > 0.5f ? b.mValue : a.mValue;
                // TODO: Implement Quadratic and TBC interpolation
                default:
                {
                    osg::Quat result;
                    result.slerp(fraction, a.mValue, b.mValue);
                    return result;
                }
            }
        }

        mutable typename MapT::MapType::const_iterator mLastLowKey;
        mutable typename MapT::MapType::const_iterator mLastHighKey;

        std::shared_ptr<const MapT> mKeys;

        ValueT mDefaultVal = ValueT();
    };

    using QuaternionInterpolator = ValueInterpolator<Nif::QuaternionKeyMap>;
    using FloatInterpolator = ValueInterpolator<Nif::FloatKeyMap>;
    using Vec3Interpolator = ValueInterpolator<Nif::Vector3KeyMap>;
    using Vec4Interpolator = ValueInterpolator<Nif::Vector4KeyMap>;
    using BoolInterpolator = ValueInterpolator<Nif::BoolKeyMap>;

    class ControllerFunction : public SceneUtil::ControllerFunction
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
        float mStopTime;
        Nif::NiTimeController::ExtrapolationMode mExtrapolationMode;

    public:
        ControllerFunction(const Nif::NiTimeController* ctrl);

        float calculate(float value) const override;

        float getMaximum() const override;
    };

    class GeomMorpherController : public SceneUtil::Controller,
                                  public SceneUtil::NodeCallback<GeomMorpherController, SceneUtil::MorphGeometry*>
    {
    public:
        GeomMorpherController(const Nif::NiGeomMorpherController* ctrl);
        GeomMorpherController();
        GeomMorpherController(const GeomMorpherController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, GeomMorpherController)

        void operator()(SceneUtil::MorphGeometry*, osg::NodeVisitor*);

    private:
        std::vector<FloatInterpolator> mKeyFrames;
        std::vector<float> mWeights;
    };

#ifdef _MSC_VER
#pragma warning(push)
    /*
     * Warning C4250: 'NifOsg::KeyframeController': inherits 'osg::Callback::osg::Callback::asCallback' via dominance,
     * there is no way to solved this if an object must inherit from both osg::Object and osg::Callback
     */
#pragma warning(disable : 4250)
#endif
    class KeyframeController : public SceneUtil::KeyframeController,
                               public SceneUtil::NodeCallback<KeyframeController, NifOsg::MatrixTransform*>
    {
    public:
        KeyframeController();
        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop);
        KeyframeController(const Nif::NiKeyframeController* keyctrl);

        META_Object(NifOsg, KeyframeController)

        osg::Vec3f getTranslation(float time) const override;
        osg::Callback* getAsCallback() override { return this; }

        KfTransform getCurrentTransformation(osg::NodeVisitor* nv) override;

        void operator()(NifOsg::MatrixTransform*, osg::NodeVisitor*);

    private:
        QuaternionInterpolator mRotations;

        FloatInterpolator mXRotations;
        FloatInterpolator mYRotations;
        FloatInterpolator mZRotations;

        Vec3Interpolator mTranslations;
        FloatInterpolator mScales;

        Nif::NiKeyframeData::AxisOrder mAxisOrder{ Nif::NiKeyframeData::AxisOrder::Order_XYZ };

        osg::Quat getXYZRotation(float time) const;
    };
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    class UVController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    public:
        UVController() = default;
        UVController(const UVController&, const osg::CopyOp&);
        UVController(const Nif::NiUVData* data, const std::set<unsigned int>& textureUnits);

        META_Object(NifOsg, UVController)

        void setDefaults(osg::StateSet* stateset) override;
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

    private:
        FloatInterpolator mUTrans;
        FloatInterpolator mVTrans;
        FloatInterpolator mUScale;
        FloatInterpolator mVScale;
        std::set<unsigned int> mTextureUnits;
    };

    class VisController : public SceneUtil::NodeCallback<VisController>, public SceneUtil::Controller
    {
    private:
        std::shared_ptr<std::vector<std::pair<float, bool>>> mData;
        BoolInterpolator mInterpolator;
        unsigned int mMask{ 0u };

        bool calculate(float time) const;

    public:
        VisController(const Nif::NiVisController* ctrl, unsigned int mask);
        VisController();
        VisController(const VisController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, VisController)

        void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };

    class RollController : public SceneUtil::NodeCallback<RollController, osg::MatrixTransform*>,
                           public SceneUtil::Controller
    {
    private:
        FloatInterpolator mData;
        double mStartingTime{ 0 };

    public:
        RollController(const Nif::NiRollController* interpolator);
        RollController() = default;
        RollController(const RollController& copy, const osg::CopyOp& copyop);

        void operator()(osg::MatrixTransform* node, osg::NodeVisitor* nv);

        META_Object(NifOsg, RollController)
    };

    class AlphaController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        FloatInterpolator mData;
        osg::ref_ptr<const osg::Material> mBaseMaterial;

    public:
        AlphaController(const Nif::NiAlphaController* ctrl, const osg::Material* baseMaterial);
        AlphaController();
        AlphaController(const AlphaController& copy, const osg::CopyOp& copyop);

        void setDefaults(osg::StateSet* stateset) override;

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

        META_Object(NifOsg, AlphaController)
    };

    class MaterialColorController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    public:
        MaterialColorController(const Nif::NiMaterialColorController* ctrl, const osg::Material* baseMaterial);
        MaterialColorController();
        MaterialColorController(const MaterialColorController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, MaterialColorController)

        void setDefaults(osg::StateSet* stateset) override;

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

    private:
        Vec3Interpolator mData;
        Nif::NiMaterialColorController::TargetColor mTargetColor{
            Nif::NiMaterialColorController::TargetColor::Ambient
        };
        osg::ref_ptr<const osg::Material> mBaseMaterial;
    };

    class FlipController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        int mTexSlot{ 0 };
        float mDelta{ 0.f };
        std::vector<osg::ref_ptr<osg::Texture2D>> mTextures;
        FloatInterpolator mData;

    public:
        FlipController(const Nif::NiFlipController* ctrl, const std::vector<osg::ref_ptr<osg::Texture2D>>& textures);
        FlipController(int texSlot, float delta, const std::vector<osg::ref_ptr<osg::Texture2D>>& textures);
        FlipController() = default;
        FlipController(const FlipController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, FlipController)

        std::vector<osg::ref_ptr<osg::Texture2D>>& getTextures() { return mTextures; }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;
    };

    class ParticleSystemController
        : public SceneUtil::NodeCallback<ParticleSystemController, osgParticle::ParticleProcessor*>,
          public SceneUtil::Controller
    {
    public:
        ParticleSystemController(const Nif::NiParticleSystemController* ctrl);
        ParticleSystemController() = default;
        ParticleSystemController(const ParticleSystemController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, ParticleSystemController)

        void operator()(osgParticle::ParticleProcessor* node, osg::NodeVisitor* nv);

    private:
        float mEmitStart{ 0.f };
        float mEmitStop{ 0.f };
    };

    class PathController : public SceneUtil::NodeCallback<PathController, NifOsg::MatrixTransform*>,
                           public SceneUtil::Controller
    {
    public:
        PathController(const Nif::NiPathController* ctrl);
        PathController() = default;
        PathController(const PathController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, PathController)

        void operator()(NifOsg::MatrixTransform*, osg::NodeVisitor*);

    private:
        Vec3Interpolator mPath;
        FloatInterpolator mPercent;
        int mFlags{ 0 };

        float getPercent(float time) const;
    };

}

#endif
