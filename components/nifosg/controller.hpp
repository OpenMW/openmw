#ifndef COMPONENTS_NIFOSG_CONTROLLER_H
#define COMPONENTS_NIFOSG_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <set> //UVController

// FlipController
#include <osg/Texture2D>
#include <osg/ref_ptr>

#include <osg/StateSet>
#include <osg/NodeCallback>
#include <osg/Drawable>


namespace osg
{
    class Node;
    class StateSet;
}

namespace osgParticle
{
    class Emitter;
}

namespace NifOsg
{

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

            return mKeys->mKeys.lower_bound(time);
        }

    public:
        using ValueT = typename MapT::ValueType;

        ValueInterpolator() = default;

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

            const typename MapT::MapType & keys = mKeys->mKeys;

            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            typename MapT::MapType::const_iterator it = retrieveKey(time);

            // now do the actual interpolation
            if (it != keys.end())
            {
                // cache for next time
                mLastHighKey = it;
                mLastLowKey = --it;

                float a = (time - mLastLowKey->first) / (mLastHighKey->first - mLastLowKey->first);

                return interpolate(mLastLowKey->second, mLastHighKey->second, a, mKeys->mInterpolationType);
            }

            return keys.rbegin()->second.mValue;
        }

        bool empty() const
        {
            return !mKeys || mKeys->mKeys.empty();
        }

    private:
        template <typename ValueType>
        ValueType interpolate(const Nif::KeyT<ValueType>& a, const Nif::KeyT<ValueType>& b, float fraction, unsigned int type) const
        {
            switch (type)
            {
                case Nif::InterpolationType_Constant:
                    return fraction > 0.5f ? b.mValue : a.mValue;
                default:
                    return a.mValue + ((b.mValue - a.mValue) * fraction);
            }
        }
        osg::Quat interpolate(const Nif::KeyT<osg::Quat>& a, const Nif::KeyT<osg::Quat>& b, float fraction, unsigned int type) const
        {
            switch (type)
            {
                case Nif::InterpolationType_Constant:
                    return fraction > 0.5f ? b.mValue : a.mValue;
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

    class ControllerFunction : public SceneUtil::ControllerFunction
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
        float mStopTime;
        enum ExtrapolationMode
        {
            Cycle = 0,
            Reverse = 1,
            Constant = 2
        };
        ExtrapolationMode mExtrapolationMode;

    public:
        ControllerFunction(const Nif::Controller *ctrl);

        float calculate(float value) const;

        virtual float getMaximum() const;
    };

    /// Must be set on a SceneUtil::MorphGeometry.
    class GeomMorpherController : public osg::Drawable::UpdateCallback, public SceneUtil::Controller
    {
    public:
        GeomMorpherController(const Nif::NiMorphData* data);
        GeomMorpherController();
        GeomMorpherController(const GeomMorpherController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, GeomMorpherController)

        virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable);

    private:
        std::vector<FloatInterpolator> mKeyFrames;
    };

    class KeyframeController : public osg::NodeCallback, public SceneUtil::Controller
    {
    public:
        KeyframeController(const Nif::NiKeyframeData *data);
        KeyframeController();
        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, KeyframeController)

        virtual osg::Vec3f getTranslation(float time) const;

        virtual void operator() (osg::Node*, osg::NodeVisitor*);

    private:
        QuaternionInterpolator mRotations;

        FloatInterpolator mXRotations;
        FloatInterpolator mYRotations;
        FloatInterpolator mZRotations;

        Vec3Interpolator mTranslations;
        FloatInterpolator mScales;

        osg::Quat getXYZRotation(float time) const;
    };

    class UVController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    public:
        UVController();
        UVController(const UVController&,const osg::CopyOp&);
        UVController(const Nif::NiUVData *data, const std::set<int>& textureUnits);

        META_Object(NifOsg,UVController)

        virtual void setDefaults(osg::StateSet* stateset);
        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);

    private:
        FloatInterpolator mUTrans;
        FloatInterpolator mVTrans;
        FloatInterpolator mUScale;
        FloatInterpolator mVScale;
        std::set<int> mTextureUnits;
    };

    class VisController : public osg::NodeCallback, public SceneUtil::Controller
    {
    private:
        std::vector<Nif::NiVisData::VisData> mData;

        bool calculate(float time) const;

    public:
        VisController(const Nif::NiVisData *data);
        VisController();
        VisController(const VisController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, VisController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);
    };

    class RollController : public osg::NodeCallback, public SceneUtil::Controller
    {
    private:
        FloatInterpolator mData;
        double mStartingTime;

    public:
        RollController(const Nif::NiFloatData *data);
        RollController();
        RollController(const RollController& copy, const osg::CopyOp& copyop);

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);

        META_Object(NifOsg, RollController)
    };

    class AlphaController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        FloatInterpolator mData;

    public:
        AlphaController(const Nif::NiFloatData *data);
        AlphaController();
        AlphaController(const AlphaController& copy, const osg::CopyOp& copyop);

        virtual void setDefaults(osg::StateSet* stateset);

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv);

        META_Object(NifOsg, AlphaController)
    };

    class MaterialColorController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    public:
        enum TargetColor
        {
            Ambient  = 0,
            Diffuse  = 1,
            Specular = 2,
            Emissive = 3
        };
        MaterialColorController(const Nif::NiPosData *data, TargetColor color);
        MaterialColorController();
        MaterialColorController(const MaterialColorController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, MaterialColorController)

        virtual void setDefaults(osg::StateSet* stateset);

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv);

    private:
        Vec3Interpolator mData;
        TargetColor mTargetColor = Ambient;
    };

    class FlipController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        int mTexSlot;
        float mDelta;
        std::vector<osg::ref_ptr<osg::Texture2D> > mTextures;

    public:
        FlipController(const Nif::NiFlipController* ctrl, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures);
        FlipController(int texSlot, float delta, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures);
        FlipController();
        FlipController(const FlipController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, FlipController)

        std::vector<osg::ref_ptr<osg::Texture2D> >& getTextures() { return mTextures; }

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);
    };

    class ParticleSystemController : public osg::NodeCallback, public SceneUtil::Controller
    {
    public:
        ParticleSystemController(const Nif::NiParticleSystemController* ctrl);
        ParticleSystemController();
        ParticleSystemController(const ParticleSystemController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, ParticleSystemController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);

    private:
        float mEmitStart;
        float mEmitStop;
    };

}

#endif
