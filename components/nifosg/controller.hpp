#ifndef COMPONENTS_NIFOSG_CONTROLLER_H
#define COMPONENTS_NIFOSG_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/statesetupdater.hpp>

#include <boost/shared_ptr.hpp>

#include <set> //UVController

// FlipController
#include <osg/Texture2D>
#include <osg/ref_ptr>

#include <osg/Timer>
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

namespace osgAnimation
{
    class MorphGeometry;
}

namespace NifOsg
{

    // interpolation of keyframes
    template <typename MapT, typename InterpolationFunc>
    class ValueInterpolator
    {
    public:
        typedef typename MapT::ValueType ValueT;
        typedef typename MapT::MapType InnerMapType;

        ValueInterpolator()
            : mDefaultVal(ValueT())
        {
        }

        ValueInterpolator(boost::shared_ptr<MapT> keys, ValueT defaultVal = ValueT())
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

            // retrieve the current position in the map, optimized for the most common case
            // where time moves linearly along the keyframe track
            typename MapT::MapType::const_iterator it = mLastHighKey;
            if (mLastHighKey != keys.end())
            {
                if (time > mLastHighKey->first)
                {
                    // try if we're there by incrementing one
                    ++mLastLowKey;
                    ++mLastHighKey;
                    it = mLastHighKey;
                }
                if (mLastHighKey == keys.end() || (time < mLastLowKey->first || time > mLastHighKey->first))
                    it = keys.lower_bound(time); // still not there, reorient by performing lower_bound check on the whole map
            }
            else
                it = keys.lower_bound(time);

            // now do the actual interpolation
            if (it != keys.end())
            {
                float aTime = it->first;
                const typename MapT::KeyType* aKey = &it->second;

                // cache for next time
                mLastHighKey = it;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                typename MapT::MapType::const_iterator last = --it;
                mLastLowKey = last;
                float aLastTime = last->first;
                const typename MapT::KeyType* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);

                return InterpolationFunc()(aLastKey->mValue, aKey->mValue, a);
            }
            else
                return keys.rbegin()->second.mValue;
        }

        bool empty() const
        {
            return !mKeys || mKeys->mKeys.empty();
        }

        // For serialization.
        inline void initMapTPtr() { mKeys = boost::shared_ptr<MapT>(new MapT); }
        inline boost::shared_ptr<MapT> getMapTPtr() const { return mKeys; }
        inline void setMapTPtr(boost::shared_ptr<MapT> p) { mKeys = p; }

    private:
        mutable typename MapT::MapType::const_iterator mLastLowKey;
        mutable typename MapT::MapType::const_iterator mLastHighKey;

        boost::shared_ptr<MapT> mKeys;

        ValueT mDefaultVal;
    };

    struct LerpFunc
    {
        template <typename ValueType>
        inline ValueType operator()(const ValueType& a, const ValueType& b, float fraction)
        {
            return a + ((b - a) * fraction);
        }
    };

    struct QuaternionSlerpFunc
    {
        inline osg::Quat operator()(const osg::Quat& a, const osg::Quat& b, float fraction)
        {
            osg::Quat result;
            result.slerp(fraction, a, b);
            return result;
        }
    };

    typedef ValueInterpolator<Nif::QuaternionKeyMap, QuaternionSlerpFunc> QuaternionInterpolator;
    typedef ValueInterpolator<Nif::FloatKeyMap, LerpFunc> FloatInterpolator;
    typedef ValueInterpolator<Nif::Vector3KeyMap, LerpFunc> Vec3Interpolator;

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

    /// Must be set on an osgAnimation::MorphGeometry.
    class GeomMorpherController : public osg::Drawable::UpdateCallback, public SceneUtil::Controller
    {
    public:
        GeomMorpherController(const Nif::NiMorphData* data);
        GeomMorpherController();
        GeomMorpherController(const GeomMorpherController& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, GeomMorpherController)

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

        META_Object(OpenMW, KeyframeController)

        virtual osg::Vec3f getTranslation(float time) const;

        virtual void operator() (osg::Node*, osg::NodeVisitor*);

        inline const QuaternionInterpolator& getRotations() const { return mRotations; }
        inline void setRotations(const QuaternionInterpolator& i) { mRotations = i; }

        QuaternionInterpolator mRotations;

        FloatInterpolator mXRotations;
        FloatInterpolator mYRotations;
        FloatInterpolator mZRotations;

        Vec3Interpolator mTranslations;
        FloatInterpolator mScales;
    private:

        osg::Quat getXYZRotation(float time) const;
    };

    class UVController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    public:
        UVController();
        UVController(const UVController&,const osg::CopyOp&);
        UVController(const Nif::NiUVData *data, std::set<int> textureUnits);

        META_Object(OpenMW, UVController)

        virtual void setDefaults(osg::StateSet* stateset);
        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);

        //inline const std::set<int>& getTextureUnits() const { return mTextureUnits; }
        //inline void setTextureUnits(const std::set<int>& tu) { mTextureUnits = tu; }

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

        META_Object(OpenMW, VisController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);
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

        META_Object(OpenMW, AlphaController)
    };

    class MaterialColorController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        Vec3Interpolator mData;

    public:
        MaterialColorController(const Nif::NiPosData *data);
        MaterialColorController();
        MaterialColorController(const MaterialColorController& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, MaterialColorController)

        virtual void setDefaults(osg::StateSet* stateset);

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv);
    };

    class FlipController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller
    {
    private:
        int mTexSlot;
        float mDelta;
        std::vector<osg::ref_ptr<osg::Texture2D> > mTextures;

    public:
        FlipController(const Nif::NiFlipController* ctrl, std::vector<osg::ref_ptr<osg::Texture2D> > textures);
        FlipController(int texSlot, float delta, std::vector<osg::ref_ptr<osg::Texture2D> > textures);
        FlipController();
        FlipController(const FlipController& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, FlipController)

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);
    };

    class ParticleSystemController : public osg::NodeCallback, public SceneUtil::Controller
    {
    public:
        ParticleSystemController(const Nif::NiParticleSystemController* ctrl);
        ParticleSystemController();
        ParticleSystemController(const ParticleSystemController& copy, const osg::CopyOp& copyop);

        META_Object(OpenMW, ParticleSystemController)

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv);

    private:
        float mEmitStart;
        float mEmitStop;
    };

}

#endif
