#ifndef OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H
#define OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H

#include <memory>

namespace VFS
{
    class Manager;
}

namespace Resource
{

    class SceneManager;

    /// @brief Wrapper class that constructs and provides access to the various resource subsystems.
    class ResourceSystem
    {
    public:
        ResourceSystem(const VFS::Manager* vfs);

        SceneManager* getSceneManager();

        const VFS::Manager* getVFS() const;

    private:
        std::auto_ptr<SceneManager> mSceneManager;

        const VFS::Manager* mVFS;
    };

}

#endif
