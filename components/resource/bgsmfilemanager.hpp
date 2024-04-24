#ifndef OPENMW_COMPONENTS_RESOURCE_BGSMFILEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_BGSMFILEMANAGER_H

#include <components/bgsm/file.hpp>

#include "resourcemanager.hpp"

namespace Resource
{

    /// @brief Handles caching of material files.
    /// @note May be used from any thread.
    class BgsmFileManager : public ResourceManager
    {
    public:
        BgsmFileManager(const VFS::Manager* vfs, double expiryDelay);
        ~BgsmFileManager() = default;

        /// Retrieve a material file from the cache or load it from the VFS if not cached yet.
        Bgsm::MaterialFilePtr get(VFS::Path::NormalizedView name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;
    };

}

#endif
