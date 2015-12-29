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
    class TextureManager;
    class NifFileManager;
    class KeyframeManager;

    /// @brief Wrapper class that constructs and provides access to the most commonly used resource subsystems.
    /// @par Resource subsystems can be used with multiple OpenGL contexts, just like the OSG equivalents, but
    ///     are built around the use of a single virtual file system.
    class ResourceSystem
    {
    public:
        ResourceSystem(const VFS::Manager* vfs);
        ~ResourceSystem();

        SceneManager* getSceneManager();
        TextureManager* getTextureManager();
        NifFileManager* getNifFileManager();
        KeyframeManager* getKeyframeManager();

        /// Indicates to each resource manager to clear the cache, i.e. to drop cached objects that are no longer referenced.
        void clearCache();

        const VFS::Manager* getVFS() const;

    private:
        std::auto_ptr<SceneManager> mSceneManager;
        std::auto_ptr<TextureManager> mTextureManager;
        std::auto_ptr<NifFileManager> mNifFileManager;
        std::auto_ptr<KeyframeManager> mKeyframeManager;

        const VFS::Manager* mVFS;

        ResourceSystem(const ResourceSystem&);
        void operator = (const ResourceSystem&);
    };

}

#endif
