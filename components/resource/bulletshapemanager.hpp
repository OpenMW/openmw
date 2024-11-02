#ifndef OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H
#define OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H

#include <osg/ref_ptr>

#include <components/vfs/pathutil.hpp>

#include "bulletshape.hpp"
#include "resourcemanager.hpp"

namespace Resource
{
    class SceneManager;
    class NifFileManager;

    struct BulletShape;
    class BulletShapeInstance;

    class MultiObjectCache;

    /// Handles loading, caching and "instancing" of bullet shapes.
    /// A shape 'instance' is a clone of another shape, with the goal of setting a different scale on this instance.
    /// @note May be used from any thread.
    class BulletShapeManager : public ResourceManager
    {
    public:
        BulletShapeManager(
            const VFS::Manager* vfs, SceneManager* sceneMgr, NifFileManager* nifFileManager, double expiryDelay);
        ~BulletShapeManager();

        /// @note May return a null pointer if the object has no shape.
        osg::ref_ptr<const BulletShape> getShape(VFS::Path::NormalizedView name);

        /// Create an instance of the given shape and cache it for later use, so that future calls to getInstance() can
        /// simply return the cached instance instead of having to create a new one.
        /// @note The returned ref_ptr may be kept by the caller to ensure that the instance stays in cache for as long
        /// as needed.
        osg::ref_ptr<BulletShapeInstance> cacheInstance(VFS::Path::NormalizedView name);

        /// @note May return a null pointer if the object has no shape.
        osg::ref_ptr<BulletShapeInstance> getInstance(VFS::Path::NormalizedView name);

        /// @see ResourceManager::updateCache
        void updateCache(double referenceTime) override;

        void clearCache() override;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        osg::ref_ptr<BulletShapeInstance> createInstance(VFS::Path::NormalizedView name);

        osg::ref_ptr<MultiObjectCache> mInstanceCache;
        SceneManager* mSceneManager;
        NifFileManager* mNifFileManager;
    };

}

#endif
