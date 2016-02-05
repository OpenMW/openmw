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
    }

    ResourceSystem::~ResourceSystem()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types
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

    void ResourceSystem::clearCache()
    {
        mNifFileManager->clearCache();
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

}
