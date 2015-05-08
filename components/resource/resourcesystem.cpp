#include "resourcesystem.hpp"

#include "scenemanager.hpp"
#include "texturemanager.hpp"

namespace Resource
{

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mTextureManager.reset(new TextureManager(vfs));
        mSceneManager.reset(new SceneManager(vfs, mTextureManager.get()));
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

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

}
