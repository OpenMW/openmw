#include "lightmanager.hpp"

#include <osg/NodeVisitor>
#include <osg/Geode>

#include <osgUtil/CullVisitor>

#include <components/sceneutil/util.hpp>

#include <boost/functional/hash.hpp>

#include <iostream>
#include <stdexcept>

namespace SceneUtil
{

    // Resets the modelview matrix to just the view matrix before applying lights.
    class LightStateAttribute : public osg::StateAttribute
    {
    public:
        LightStateAttribute() : mIndex(0) {}
        LightStateAttribute(unsigned int index, const std::vector<osg::ref_ptr<osg::Light> >& lights) : mIndex(index), mLights(lights) {}

        LightStateAttribute(const LightStateAttribute& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mIndex(copy.mIndex), mLights(copy.mLights) {}

        unsigned int getMember() const
        {
            return mIndex;
        }

        virtual bool getModeUsage(ModeUsage & usage) const
        {
            for (unsigned int i=0; i<mLights.size(); ++i)
                usage.usesMode(GL_LIGHT0 + mIndex + i);
            return true;
        }

        virtual int compare(const StateAttribute &sa) const
        {
            throw std::runtime_error("LightStateAttribute::compare: unimplemented");
        }

        META_StateAttribute(NifOsg, LightStateAttribute, osg::StateAttribute::LIGHT)

        virtual void apply(osg::State& state) const
        {
            osg::Matrix modelViewMatrix = state.getModelViewMatrix();

            state.applyModelViewMatrix(state.getInitialViewMatrix());

            for (unsigned int i=0; i<mLights.size(); ++i)
            {
                mLights[i]->setLightNum(i+mIndex);
                mLights[i]->apply(state);
            }

            state.applyModelViewMatrix(modelViewMatrix);
        }

    private:
        unsigned int mIndex;

        std::vector<osg::ref_ptr<osg::Light> > mLights;
    };

    // Set on a LightSource. Adds the light source to its light manager for the current frame.
    // This allows us to keep track of the current lights in the scene graph without tying creation & destruction to the manager.
    class CollectLightCallback : public osg::NodeCallback
    {
    public:
        CollectLightCallback()
            : mLightManager(0) { }

        CollectLightCallback(const CollectLightCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            , mLightManager(0) { }

        META_Object(SceneUtil, SceneUtil::CollectLightCallback)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mLightManager)
            {
                for (unsigned int i=0;i<nv->getNodePath().size(); ++i)
                {
                    if (LightManager* lightManager = dynamic_cast<LightManager*>(nv->getNodePath()[i]))
                    {
                        mLightManager = lightManager;
                        break;
                    }
                }
                if (!mLightManager)
                    throw std::runtime_error("can't find parent LightManager");
            }

            mLightManager->addLight(static_cast<LightSource*>(node), osg::computeLocalToWorld(nv->getNodePath()));

            traverse(node, nv);
        }

    private:
        LightManager* mLightManager;
    };

    // Set on a LightManager. Clears the data from the previous frame.
    class LightManagerUpdateCallback : public osg::NodeCallback
    {
    public:
        LightManagerUpdateCallback()
            { }

        LightManagerUpdateCallback(const LightManagerUpdateCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            { }

        META_Object(SceneUtil, SceneUtil::LightManagerUpdateCallback)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            LightManager* lightManager = static_cast<LightManager*>(node);
            lightManager->update();

            traverse(node, nv);
        }
    };

    LightManager::LightManager()
        : mLightsInViewSpace(false)
        , mStartLight(0)
    {
        setUpdateCallback(new LightManagerUpdateCallback);
    }

    LightManager::LightManager(const LightManager &copy, const osg::CopyOp &copyop)
        : osg::Group(copy, copyop)
        , mLightsInViewSpace(false)
        , mStartLight(copy.mStartLight)
    {

    }

    void LightManager::update()
    {
        mLightsInViewSpace = false;
        mLights.clear();
        mStateSetCache.clear();
    }

    void LightManager::addLight(LightSource* lightSource, osg::Matrix worldMat)
    {
        LightSourceTransform l;
        l.mLightSource = lightSource;
        l.mWorldMatrix = worldMat;
        lightSource->getLight()->setPosition(osg::Vec4f(worldMat.getTrans().x(),
                                                        worldMat.getTrans().y(),
                                                        worldMat.getTrans().z(), 1.f));
        mLights.push_back(l);
    }

    void LightManager::prepareForCamera(osg::Camera *cam)
    {
        // later on we need to store this per camera
        if (!mLightsInViewSpace)
        {
            for (std::vector<LightSourceTransform>::iterator it = mLights.begin(); it != mLights.end(); ++it)
            {
                LightSourceTransform& l = *it;
                osg::Matrix worldViewMat = l.mWorldMatrix * cam->getViewMatrix();
                l.mViewBound = osg::BoundingSphere(osg::Vec3f(0,0,0), l.mLightSource->getRadius());
                transformBoundingSphere(worldViewMat, l.mViewBound);
            }
            mLightsInViewSpace = true;
        }
    }

    osg::ref_ptr<osg::StateSet> LightManager::getLightListStateSet(const LightList &lightList)
    {
        // possible optimization: return a StateSet containing all requested lights plus some extra lights (if a suitable one exists)
        size_t hash = 0;
        for (unsigned int i=0; i<lightList.size();++i)
            boost::hash_combine(hash, lightList[i]);

        LightStateSetMap::iterator found = mStateSetCache.find(hash);
        if (found != mStateSetCache.end())
            return found->second;
        else
        {

            std::vector<osg::ref_ptr<osg::Light> > lights;
            for (unsigned int i=0; i<lightList.size();++i)
            {
                const LightSourceTransform& l = mLights[lightList[i]];
                lights.push_back(l.mLightSource->getLight());
            }

            osg::ref_ptr<LightStateAttribute> attr = new LightStateAttribute(mStartLight, lights);

            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;

            // don't use setAttributeAndModes, that does not support light indices!
            stateset->setAttribute(attr, osg::StateAttribute::ON);
            stateset->setAssociatedModes(attr, osg::StateAttribute::ON);

            mStateSetCache.insert(std::make_pair(hash, stateset));
            return stateset;
        }
    }

    const std::vector<LightManager::LightSourceTransform>& LightManager::getLights() const
    {
        return mLights;
    }

    void LightManager::setStartLight(int start)
    {
        mStartLight = start;
    }

    int LightManager::getStartLight() const
    {
        return mStartLight;
    }

    LightSource::LightSource()
        : mRadius(0.f)
    {
        setUpdateCallback(new CollectLightCallback);
    }

    void LightListCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

        if (!mLightManager)
        {
            for (unsigned int i=0;i<nv->getNodePath().size(); ++i)
            {
                if (LightManager* lightManager = dynamic_cast<LightManager*>(nv->getNodePath()[i]))
                {
                    mLightManager = lightManager;
                    break;
                }
            }
            if (!mLightManager)
                return;
        }

        mLightManager->prepareForCamera(cv->getCurrentCamera());

        // Possible optimizations:
        // - cull list of lights by the camera frustum
        // - organize lights in a quad tree

        const std::vector<LightManager::LightSourceTransform>& lights = mLightManager->getLights();

        if (lights.size())
        {

            static std::map<osg::Node*, osg::ref_ptr<osg::StateSet> > statesets;
            std::map<osg::Node*, osg::ref_ptr<osg::StateSet> >::iterator found = statesets.find(node);
            osg::ref_ptr<osg::StateSet> stateset;
            if (found != statesets.end())
            {
                stateset = found->second;
            }
            else{

                // we do the intersections in view space
                osg::BoundingSphere nodeBound = node->getBound();
                osg::Matrixf mat = *cv->getModelViewMatrix();
                transformBoundingSphere(mat, nodeBound);

                std::vector<int> lightList;
                for (unsigned int i=0; i<lights.size(); ++i)
                {
                    const LightManager::LightSourceTransform& l = lights[i];
                    if (l.mViewBound.intersects(nodeBound))
                        lightList.push_back(i);
                }

                if (lightList.empty())
                {
                    statesets[node] = NULL;
                    traverse(node, nv);
                    return;
                }

                unsigned int maxLights = static_cast<unsigned int> (8 - mLightManager->getStartLight());

                if (lightList.size() > maxLights)
                {
                    //std::cerr << "More than 8 lights!" << std::endl;

                    // TODO: sort lights by certain criteria

                    while (lightList.size() > maxLights)
                        lightList.pop_back();
                }

                stateset = mLightManager->getLightListStateSet(lightList);
                statesets[node] = stateset;
            }

            if (stateset)
                cv->pushStateSet(stateset);

            traverse(node, nv);

            if (stateset)
                cv->popStateSet();
        }
        else
            traverse(node, nv);
    }

}
