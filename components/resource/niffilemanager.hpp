#ifndef OPENMW_COMPONENTS_RESOURCE_NIFFILEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_NIFFILEMANAGER_H

#include <osg/ref_ptr>

#include <components/nif/niffile.hpp>

namespace VFS
{
    class Manager;
}

namespace osgDB
{
    class ObjectCache;
}

namespace Resource
{

    /// @brief Handles caching of NIFFiles.
    /// @note The NifFileManager is completely thread safe.
    class NifFileManager
    {
    public:
        NifFileManager(const VFS::Manager* vfs);
        ~NifFileManager();

        void clearCache();

        /// Retrieve a NIF file from the cache, or load it from the VFS if not cached yet.
        /// @note For performance reasons the NifFileManager does not handle case folding, needs
        /// to be done in advance by other managers accessing the NifFileManager.
        Nif::NIFFilePtr get(const std::string& name);

    private:
        // Use the osgDB::ObjectCache so objects are retrieved in thread safe way
        osg::ref_ptr<osgDB::ObjectCache> mCache;

        const VFS::Manager* mVFS;
    };

}

#endif
