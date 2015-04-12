#include "animation.hpp"

#include <components/nifosg/nifloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <osg/PositionAttitudeTransform>

namespace MWRender
{

    Animation::Animation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> node, Resource::ResourceSystem* resourceSystem)
        : mPtr(ptr)
        , mInsert(node)
        , mResourceSystem(resourceSystem)
    {

    }

    Animation::~Animation()
    {
        if (mObjectRoot)
            mInsert->removeChild(mObjectRoot);
    }

    osg::Vec3f Animation::runAnimation(float duration)
    {
        return osg::Vec3f();
    }

    void Animation::setObjectRoot(const std::string &model)
    {
        if (mObjectRoot)
        {
            mObjectRoot->getParent(0)->removeChild(mObjectRoot);
        }

        mObjectRoot = mResourceSystem->getSceneManager()->createInstance(model, mInsert);
    }

    osg::Group* Animation::getObjectRoot()
    {
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    osg::Group* Animation::getOrCreateObjectRoot()
    {
        if (mObjectRoot)
            return static_cast<osg::Group*>(mObjectRoot.get());

        mObjectRoot = new osg::Group;
        mInsert->addChild(mObjectRoot);
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    // --------------------------------------------------------------------------------

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr &ptr, const std::string &model, Resource::ResourceSystem* resourceSystem)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model);
        }
        else
        {
            // No model given. Create an object root anyway, so that lights can be added to it if needed.
            //mObjectRoot = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        }
    }

}
