#ifndef OPENMW_COMPONENTS_RESOURCE_MANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_MANAGER_H

#include <memory>

#include <osg/Node>
#include <osg/ref_ptr>

#include <components/vfs/pathutil.hpp>

#include "objectcache.hpp"

namespace VFS
{
    class Manager;
}

namespace osg
{
    class Stats;
    class State;
}

namespace Resource
{

    class BaseResourceManager
    {
    public:
        virtual ~BaseResourceManager() = default;
        virtual void updateCache(double referenceTime) = 0;
        virtual void clearCache() = 0;
        virtual void setExpiryDelay(double expiryDelay) = 0;
        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const = 0;
        virtual void releaseGLObjects(osg::State* state) = 0;
    };

    /// @brief Base class for managers that require a virtual file system and object cache.
    /// @par This base class implements clearing of the cache, but populating it and what it's used for is up to the
    /// individual sub classes.
    template <class KeyType, class ValueType>
    class GenericResourceManager : public BaseResourceManager
    {
    public:
        typedef GenericObjectCache<KeyType, ValueType> CacheType;

        explicit GenericResourceManager(const VFS::Manager* vfs, double expiryDelay)
            : mVFS(vfs)
            , mCache(std::make_unique<CacheType>())
            , mExpiryDelay(expiryDelay)
        {
        }

        virtual ~GenericResourceManager() = default;

        /// Clear cache entries that have not been referenced for longer than expiryDelay.
        void updateCache(double referenceTime) override { mCache->update(referenceTime, mExpiryDelay); }

        /// Clear all cache entries.
        void clearCache() override { mCache->clear(); }

        /// How long to keep objects in cache after no longer being referenced.
        void setExpiryDelay(double expiryDelay) final { mExpiryDelay = expiryDelay; }
        double getExpiryDelay() const { return mExpiryDelay; }

        const VFS::Manager* getVFS() const { return mVFS; }

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override {}

        void releaseGLObjects(osg::State* state) override
        {
            // Only meaningful for caches whose values are GL-backed; see objectcache.hpp.
            if constexpr (requires { mCache->releaseGLObjects(state); })
                mCache->releaseGLObjects(state);
        }

    protected:
        const VFS::Manager* mVFS;
        std::unique_ptr<CacheType> mCache;
        double mExpiryDelay;
    };

    /// Caching a scene graph is the common case, so it gets a name rather than the
    /// same template arguments at every use.
    template <class KeyType>
    using NodeResourceManager = GenericResourceManager<KeyType, osg::ref_ptr<osg::Node>>;

    template <class ValueType>
    class ResourceManager : public GenericResourceManager<std::string, ValueType>
    {
    public:
        explicit ResourceManager(const VFS::Manager* vfs, double expiryDelay)
            : GenericResourceManager<std::string, ValueType>(vfs, expiryDelay)
        {
        }

    protected:
        using GenericResourceManager<std::string, ValueType>::mCache;
        using GenericResourceManager<std::string, ValueType>::mVFS;
    };

}

#endif
