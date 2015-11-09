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

    class ValueInterpolator
    {
    protected:
        template <typename T>
        T interpKey (const std::map< float, Nif::KeyT<T> >& keys, float time, T defaultValue = T()) const
        {
            if (keys.size() == 0)
                return defaultValue;

            if(time <= keys.begin()->first)
                return keys.begin()->second.mValue;

            typename std::map< float, Nif::KeyT<T> >::const_iterator it = keys.lower_bound(time);
            if (it != keys.end())
            {
                float aTime = it->first;
                const Nif::KeyT<T>* aKey = &it->second;

                assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

                typename std::map< float, Nif::KeyT<T> >::const_iterator last = --it;
                float aLastTime = last->first;
                const Nif::KeyT<T>* aLastKey = &last->second;

                float a = (time - aLastTime) / (aTime - aLastTime);
                return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
            }
            else
                return keys.rbegin()->second.mValue;
        }
    };

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

    class GeomMorpherController : public osg::Drawable::UpdateCallback, public SceneUtil::Controller, public ValueInterpolator
    {
    public:
        GeomMorpherController(const Nif::NiMorphData* data);
        GeomMorpherController();
        GeomMorpherController(const GeomMorpherController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, GeomMorpherController)

        virtual void update(osg::NodeVisitor* nv, osg::Drawable* drawable);

    private:
        std::vector<Nif::FloatKeyMapPtr> mKeyFrames;
    };

    class KeyframeController : public osg::NodeCallback, public SceneUtil::Controller, public ValueInterpolator
    {
    public:
        KeyframeController(const Nif::NiKeyframeData *data);
        KeyframeController();
        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, KeyframeController)

        virtual osg::Vec3f getTranslation(float time) const;

        virtual void operator() (osg::Node*, osg::NodeVisitor*);

    private:
        Nif::QuaternionKeyMapPtr mRotations;

        Nif::FloatKeyMapPtr mXRotations;
        Nif::FloatKeyMapPtr mYRotations;
        Nif::FloatKeyMapPtr mZRotations;

        Nif::Vector3KeyMapPtr mTranslations;
        Nif::FloatKeyMapPtr mScales;

        using ValueInterpolator::interpKey;

        osg::Quat interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

        osg::Quat getXYZRotation(float time) const;
    };

    class UVController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller, public ValueInterpolator
    {
    public:
        UVController();
        UVController(const UVController&,const osg::CopyOp&);
        UVController(const Nif::NiUVData *data, std::set<int> textureUnits);

        META_Object(NifOsg,UVController)

        virtual void setDefaults(osg::StateSet* stateset);
        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);

    private:
        Nif::FloatKeyMapPtr mUTrans;
        Nif::FloatKeyMapPtr mVTrans;
        Nif::FloatKeyMapPtr mUScale;
        Nif::FloatKeyMapPtr mVScale;
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

    class AlphaController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller, public ValueInterpolator
    {
    private:
        Nif::FloatKeyMapPtr mData;

    public:
        AlphaController(const Nif::NiFloatData *data);
        AlphaController();
        AlphaController(const AlphaController& copy, const osg::CopyOp& copyop);

        virtual void setDefaults(osg::StateSet* stateset);

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv);

        META_Object(NifOsg, AlphaController)
    };

    class MaterialColorController : public SceneUtil::StateSetUpdater, public SceneUtil::Controller, public ValueInterpolator
    {
    private:
        Nif::Vector3KeyMapPtr mData;

    public:
        MaterialColorController(const Nif::NiPosData *data);
        MaterialColorController();
        MaterialColorController(const MaterialColorController& copy, const osg::CopyOp& copyop);

        META_Object(NifOsg, MaterialColorController)

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

        META_Object(NifOsg, FlipController)

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
