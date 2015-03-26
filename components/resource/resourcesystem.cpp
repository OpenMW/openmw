#include "resourcesystem.hpp"

#include "scenemanager.hpp"

namespace Resource
{

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mSceneManager.reset(new SceneManager(vfs));
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
