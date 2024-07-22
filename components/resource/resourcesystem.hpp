#ifndef OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H
#define OPENMW_COMPONENTS_RESOURCE_RESOURCESYSTEM_H

#include <memory>
#include <vector>

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Stats;
    class State;
}

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

namespace Resource
{

    class SceneManager;
    class ImageManager;
    class BgsmFileManager;
    class NifFileManager;
    class KeyframeManager;
    class BaseResourceManager;
    class AnimBlendRulesManager;

    /// @brief Wrapper class that constructs and provides access to the most commonly used resource subsystems.
    /// @par Resource subsystems can be used with multiple OpenGL contexts, just like the OSG equivalents, but
    ///     are built around the use of a single virtual file system.
    class ResourceSystem
    {
    public:
        explicit ResourceSystem(
            const VFS::Manager* vfs, double expiryDelay, const ToUTF8::StatelessUtf8Encoder* encoder);
        ~ResourceSystem();

        SceneManager* getSceneManager();
        ImageManager* getImageManager();
        BgsmFileManager* getBgsmFileManager();
        NifFileManager* getNifFileManager();
        KeyframeManager* getKeyframeManager();
        AnimBlendRulesManager* getAnimBlendRulesManager();

        /// Indicates to each resource manager to clear the cache, i.e. to drop cached objects that are no longer
        /// referenced.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void updateCache(double referenceTime);

        /// Indicates to each resource manager to clear the entire cache.
        /// @note May be called from any thread if you do not add or remove resource managers at that point.
        void clearCache();

        /// Add this ResourceManager to be handled by the ResourceSystem.
        /// @note Does not transfer ownership.
        void addResourceManager(BaseResourceManager* resourceMgr);
        /// @note Do nothing if resourceMgr does not exist.
        /// @note Does not delete resourceMgr.
        void removeResourceManager(BaseResourceManager* resourceMgr);

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay(double expiryDelay);

        /// @note May be called from any thread.
        const VFS::Manager* getVFS() const;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

        /// Call releaseGLObjects for each resource manager.
        void releaseGLObjects(osg::State* state);

    private:
        std::unique_ptr<SceneManager> mSceneManager;
        std::unique_ptr<ImageManager> mImageManager;
        std::unique_ptr<BgsmFileManager> mBgsmFileManager;
        std::unique_ptr<NifFileManager> mNifFileManager;
        std::unique_ptr<KeyframeManager> mKeyframeManager;
        std::unique_ptr<AnimBlendRulesManager> mAnimBlendRulesManager;

        // Store the base classes separately to get convenient access to the common interface
        // Here users can register their own resourcemanager as well
        std::vector<BaseResourceManager*> mResourceManagers;

        const VFS::Manager* mVFS;

        ResourceSystem(const ResourceSystem&);
        void operator=(const ResourceSystem&);
    };

}

#endif
