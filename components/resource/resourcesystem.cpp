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

    SceneManager* ResourceSystem::getSceneManager()
    {
        return mSceneManager.get();
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

}
