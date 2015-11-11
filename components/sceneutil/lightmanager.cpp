#include "lightmanager.hpp"

#include <stdexcept>

#include <osg/NodeVisitor>
#include <osg/Geode>

#include <osgUtil/CullVisitor>

#include <components/sceneutil/util.hpp>

#include <boost/functional/hash.hpp>

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

    LightManager* findLightManager(const osg::NodePath& path)
    {
        for (unsigned int i=0;i<path.size(); ++i)
        {
            if (LightManager* lightManager = dynamic_cast<LightManager*>(path[i]))
                return lightManager;
        }
        return NULL;
    }

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
                mLightManager = findLightManager(nv->getNodePath());

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
        : mStartLight(0)
        , mLightingMask(~0u)
    {
        setUpdateCallback(new LightManagerUpdateCallback);
    }

    LightManager::LightManager(const LightManager &copy, const osg::CopyOp &copyop)
        : osg::Group(copy, copyop)
        , mStartLight(copy.mStartLight)
        , mLightingMask(copy.mLightingMask)
    {

    }

    void LightManager::setLightingMask(unsigned int mask)
    {
        mLightingMask = mask;
    }

    unsigned int LightManager::getLightingMask() const
    {
        return mLightingMask;
    }

    void LightManager::update()
    {
        mLights.clear();
        mLightsInViewSpace.clear();

        // do an occasional cleanup for orphaned lights
        if (mStateSetCache.size() > 5000)
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

    osg::ref_ptr<osg::StateSet> LightManager::getLightListStateSet(const LightList &lightList)
    {
        // possible optimization: return a StateSet containing all requested lights plus some extra lights (if a suitable one exists)
        size_t hash = 0;
        for (unsigned int i=0; i<lightList.size();++i)
            boost::hash_combine(hash, lightList[i]->mLightSource->getId());

        LightStateSetMap::iterator found = mStateSetCache.find(hash);
        if (found != mStateSetCache.end())
            return found->second;
        else
        {

            std::vector<osg::ref_ptr<osg::Light> > lights;
            for (unsigned int i=0; i<lightList.size();++i)
                lights.push_back(lightList[i]->mLightSource->getLight());

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

    const std::vector<LightManager::LightSourceViewBound>& LightManager::getLightsInViewSpace(osg::Camera *camera, const osg::RefMatrix* viewMatrix)
    {
        osg::observer_ptr<osg::Camera> camPtr (camera);
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection>::iterator it = mLightsInViewSpace.find(camPtr);

        if (it == mLightsInViewSpace.end())
        {
            it = mLightsInViewSpace.insert(std::make_pair(camPtr, LightSourceViewBoundCollection())).first;

            for (std::vector<LightSourceTransform>::iterator lightIt = mLights.begin(); lightIt != mLights.end(); ++lightIt)
            {
                osg::Matrix worldViewMat = lightIt->mWorldMatrix * (*viewMatrix);
                osg::BoundingSphere viewBound = osg::BoundingSphere(osg::Vec3f(0,0,0), lightIt->mLightSource->getRadius());
                transformBoundingSphere(worldViewMat, viewBound);

                LightSourceViewBound l;
                l.mLightSource = lightIt->mLightSource;
                l.mViewBound = viewBound;
                it->second.push_back(l);
            }
        }
        return it->second;
    }

    void LightManager::setStartLight(int start)
    {
        mStartLight = start;
    }

    int LightManager::getStartLight() const
    {
        return mStartLight;
    }

    static int sLightId = 0;

    LightSource::LightSource()
        : mRadius(0.f)
    {
        setUpdateCallback(new CollectLightCallback);
        mId = sLightId++;
    }

    LightSource::LightSource(const LightSource &copy, const osg::CopyOp &copyop)
        : osg::Node(copy, copyop)
        , mLight(copy.mLight)
        , mRadius(copy.mRadius)
    {
        mId = sLightId++;
    }


    bool sortLights (const LightManager::LightSourceViewBound* left, const LightManager::LightSourceViewBound* right)
    {
        return left->mViewBound.center().length2() - left->mViewBound.radius2()/4.f < right->mViewBound.center().length2() - right->mViewBound.radius2()/4.f;
    }

    void LightListCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

        if (!mLightManager)
        {
            mLightManager = findLightManager(nv->getNodePath());
            if (!mLightManager)
            {
                traverse(node, nv);
                return;
            }
        }

        if (!(cv->getCurrentCamera()->getCullMask() & mLightManager->getLightingMask()))
        {
            traverse(node, nv);
            return;
        }

        // Possible optimizations:
        // - cull list of lights by the camera frustum
        // - organize lights in a quad tree


        // update light list if necessary
        // makes sure we don't update it more than once per frame when rendering with multiple cameras
        if (mLastFrameNumber != nv->getFrameStamp()->getFrameNumber())
        {
            mLastFrameNumber = nv->getFrameStamp()->getFrameNumber();

            // Don't use Camera::getViewMatrix, that one might be relative to another camera!
            const osg::RefMatrix* viewMatrix = cv->getCurrentRenderStage()->getInitialViewMatrix();
            const std::vector<LightManager::LightSourceViewBound>& lights = mLightManager->getLightsInViewSpace(cv->getCurrentCamera(), viewMatrix);

            // we do the intersections in view space
            osg::BoundingSphere nodeBound = node->getBound();
            osg::Matrixf mat = *cv->getModelViewMatrix();
            transformBoundingSphere(mat, nodeBound);

            mLightList.clear();
            for (unsigned int i=0; i<lights.size(); ++i)
            {
                const LightManager::LightSourceViewBound& l = lights[i];
                if (l.mViewBound.intersects(nodeBound))
                    mLightList.push_back(&l);
            }
        }
        if (mLightList.size())
        {
            unsigned int maxLights = static_cast<unsigned int> (8 - mLightManager->getStartLight());

            osg::StateSet* stateset = NULL;

            if (mLightList.size() > maxLights)
            {
                // remove lights culled by this camera
                LightManager::LightList lightList = mLightList;
                for (LightManager::LightList::iterator it = lightList.begin(); it != lightList.end() && lightList.size() > maxLights; )
                {
                    osg::CullStack::CullingStack& stack = cv->getModelViewCullingStack();

                    osg::BoundingSphere bs = (*it)->mViewBound;
                    bs._radius = bs._radius*2;
                    osg::CullingSet& cullingSet = stack.front();
                    if (cullingSet.isCulled(bs))
                    {
                        it = lightList.erase(it);
                        continue;
                    }
                    else
                        ++it;
                }

                if (lightList.size() > maxLights)
                {
                    // sort by proximity to camera, then get rid of furthest away lights
                    std::sort(lightList.begin(), lightList.end(), sortLights);
                    while (lightList.size() > maxLights)
                        lightList.pop_back();
                }
                stateset = mLightManager->getLightListStateSet(lightList);
            }
            else
                stateset = mLightManager->getLightListStateSet(mLightList);


            cv->pushStateSet(stateset);

            traverse(node, nv);

            cv->popStateSet();
        }
        else
            traverse(node, nv);
    }

    void configureLight(osg::Light *light, float radius, bool isExterior, bool outQuadInLin, bool useQuadratic,
                        float quadraticValue, float quadraticRadiusMult, bool useLinear, float linearRadiusMult, float linearValue)
    {
        bool quadratic = useQuadratic && (!outQuadInLin || isExterior);

        float quadraticAttenuation = 0;
        float linearAttenuation = 0;
        if (quadratic)
        {
            float r = radius * quadraticRadiusMult;
            quadraticAttenuation = quadraticValue / std::pow(r, 2);
        }
        if (useLinear)
        {
            float r = radius * linearRadiusMult;
            linearAttenuation = linearValue / r;
        }

        light->setLinearAttenuation(linearAttenuation);
        light->setQuadraticAttenuation(quadraticAttenuation);
        light->setConstantAttenuation(0.f);

    }

}
