#ifndef COMPONENTS_NIFOSG_CONTROLLER_H
#define COMPONENTS_NIFOSG_CONTROLLER_H

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>

#include <components/nifcache/nifcache.hpp>

#include <boost/shared_ptr.hpp>

#include <set> //UVController

#include <osg/Timer>


namespace osg
{
    class Node;
    class StateSet;
}

namespace osgAnimation
{
    class MorphGeometry;
}

namespace NifOsg
{

    // FIXME: Should not be here. We might also want to use this for non-NIF model formats
    class ValueInterpolator
    {
    protected:
        float interpKey(const Nif::FloatKeyMap::MapType &keys, float time, float def=0.f) const;

        osg::Vec3f interpKey(const Nif::Vector3KeyMap::MapType &keys, float time) const;
    };

    // FIXME: Should not be here. We might also want to use this for non-NIF model formats
    class ControllerFunction
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
        bool mDeltaInput;
        float mDeltaCount;
    public:
        float mStopTime;

    public:
        ControllerFunction(const Nif::Controller *ctrl, bool deltaInput);

        float calculate(float value);
    };
    typedef ControllerFunction DefaultFunction;

    class ControllerSource
    {
    public:
        virtual float getValue() const = 0;
    };

    // FIXME: Should return a dt instead of time
    class FrameTimeSource : public ControllerSource
    {
    public:

        virtual float getValue() const
        {
            return mTimer.time_s();
        }

    private:
        osg::Timer mTimer;
    };

    class ControllerValue
    {
    public:
        virtual void setValue(float value) = 0;
    };

    class Controller
    {
    public:
        Controller (boost::shared_ptr<ControllerSource> src, boost::shared_ptr<ControllerValue> dest,
                    boost::shared_ptr<ControllerFunction> function);

        virtual void update();

        boost::shared_ptr<ControllerSource> mSource;
        boost::shared_ptr<ControllerValue> mDestValue;

        // The source value gets passed through this function before it's passed on to the DestValue.
        boost::shared_ptr<ControllerFunction> mFunction;
    };

    // FIXME: Should be with other general extensions.
    class NodeTargetValue : public ControllerValue
    {
    protected:
        osg::Node *mNode;

    public:
        NodeTargetValue(osg::Node *target) : mNode(target)
        { }

        virtual osg::Vec3f getTranslation(float value) const = 0;

        osg::Node *getNode() const
        { return mNode; }
    };

    class GeomMorpherControllerValue : public ControllerValue, public ValueInterpolator
    {
    public:
        // FIXME: don't copy the morph data?
        GeomMorpherControllerValue(osgAnimation::MorphGeometry* geom, const Nif::NiMorphData *data);

        virtual void setValue(float time);

    private:
        osgAnimation::MorphGeometry* mGeom;
        std::vector<Nif::NiMorphData::MorphData> mMorphs;
    };

    class KeyframeControllerValue : public NodeTargetValue, public ValueInterpolator
    {
    private:
        const Nif::QuaternionKeyMap* mRotations;
        const Nif::FloatKeyMap* mXRotations;
        const Nif::FloatKeyMap* mYRotations;
        const Nif::FloatKeyMap* mZRotations;
        const Nif::Vector3KeyMap* mTranslations;
        const Nif::FloatKeyMap* mScales;
        Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid

        osg::Quat mInitialQuat;
        float mInitialScale;

        using ValueInterpolator::interpKey;


        osg::Quat interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

        osg::Quat getXYZRotation(float time) const;

    public:
        /// @note The NiKeyFrameData must be valid as long as this KeyframeController exists.
        KeyframeControllerValue(osg::Node *target, const Nif::NIFFilePtr& nif, const Nif::NiKeyframeData *data,
              osg::Quat initialQuat, float initialScale);

        virtual osg::Vec3f getTranslation(float time) const;

        virtual void setValue(float time);
    };

    class UVControllerValue : public ControllerValue, ValueInterpolator
    {
    private:
        osg::StateSet* mStateSet;
        Nif::FloatKeyMap mUTrans;
        Nif::FloatKeyMap mVTrans;
        Nif::FloatKeyMap mUScale;
        Nif::FloatKeyMap mVScale;
        std::set<int> mTextureUnits;

    public:
        UVControllerValue(osg::StateSet* target, const Nif::NiUVData *data, std::set<int> textureUnits);

        virtual void setValue(float value);
    };

    class VisControllerValue : public NodeTargetValue
    {
    private:
        std::vector<Nif::NiVisData::VisData> mData;

        bool calculate(float time) const;

    public:
        VisControllerValue(osg::Node *target, const Nif::NiVisData *data)
          : NodeTargetValue(target)
          , mData(data->mVis)
        { }

        virtual osg::Vec3f getTranslation(float time) const
        { return osg::Vec3f(); }

        virtual void setValue(float time);
    };

}

#endif
