#include "resourcesystem.hpp"

#include "scenemanager.hpp"
#include "imagemanager.hpp"
#include "niffilemanager.hpp"
#include "keyframemanager.hpp"

namespace Resource
{

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mNifFileManager.reset(new NifFileManager(vfs));
        mKeyframeManager.reset(new KeyframeManager(vfs));
        mImageManager.reset(new ImageManager(vfs));
        mSceneManager.reset(new SceneManager(vfs, mImageManager.get(), mNifFileManager.get()));

        addResourceManager(mNifFileManager.get());
        addResourceManager(mKeyframeManager.get());
        // note, scene references images so add images afterwards for correct implementation of updateCache()
        addResourceManager(mSceneManager.get());
        addResourceManager(mImageManager.get());
    }

    ResourceSystem::~ResourceSystem()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types

        mResourceManagers.clear();

        // no delete, all handled by auto_ptr
    }

    SceneManager* ResourceSystem::getSceneManager()
    {
        return mSceneManager.get();
    }

    ImageManager* ResourceSystem::getImageManager()
    {
        return mImageManager.get();
    }

    NifFileManager* ResourceSystem::getNifFileManager()
    {
        return mNifFileManager.get();
    }

    KeyframeManager* ResourceSystem::getKeyframeManager()
    {
        return mKeyframeManager.get();
    }

    void ResourceSystem::updateCache(double referenceTime)
    {
        osg::Timer timer;
        for (std::vector<ResourceManager*>::iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->updateCache(referenceTime);
        std::cout << "updateCache took " << timer.time_m() << " ms" << std::endl;
    }

    void ResourceSystem::addResourceManager(ResourceManager *resourceMgr)
    {
        mResourceManagers.push_back(resourceMgr);
    }

    void ResourceSystem::removeResourceManager(ResourceManager *resourceMgr)
    {
        std::vector<ResourceManager*>::iterator found = std::find(mResourceManagers.begin(), mResourceManagers.end(), resourceMgr);
        if (found != mResourceManagers.end())
            mResourceManagers.erase(found);
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

}
