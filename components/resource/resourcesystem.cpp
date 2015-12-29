#include "resourcesystem.hpp"

#include "scenemanager.hpp"
#include "texturemanager.hpp"
#include "niffilemanager.hpp"
#include "keyframemanager.hpp"

namespace Resource
{

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mNifFileManager.reset(new NifFileManager(vfs));
        mKeyframeManager.reset(new KeyframeManager(vfs));
        mTextureManager.reset(new TextureManager(vfs));
        mSceneManager.reset(new SceneManager(vfs, mTextureManager.get(), mNifFileManager.get()));
    }

    ResourceSystem::~ResourceSystem()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types
    }

    SceneManager* ResourceSystem::getSceneManager()
    {
        return mSceneManager.get();
    }

    TextureManager* ResourceSystem::getTextureManager()
    {
        return mTextureManager.get();
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
