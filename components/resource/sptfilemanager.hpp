#ifndef OPENMW_COMPONENTS_RESOURCE_SPTFILEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCE_SPTFILEMANAGER_H

#include <components/spt/file.hpp>

#include "resourcemanager.hpp"

namespace Resource
{
    class SptFileManager : public ResourceManager
    {
    public:
        SptFileManager(const VFS::Manager* vfs, double expiryDelay);
        ~SptFileManager() = default;

        Spt::SPTFilePtr get(VFS::Path::NormalizedView name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;
    };
}

#endif