#include "sptfilemanager.hpp"

#include <memory>

#include <components/vfs/manager.hpp>

#include "components/spt/file.hpp"
#include "objectcache.hpp"

namespace Resource
{
    SptFileManager::SptFileManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
    {
    }

    Spt::SPTFilePtr SptFileManager::get(VFS::Path::NormalizedView name)
    {
        if (Spt::SPTFilePtr cached = mCache->getRefFromObjectCache(name))
            return cached;

        Spt::SPTFilePtr file = Spt::parse(mVFS->get(name), name.value());
        mCache->addEntryToObjectCache(name.value(), file);
        return file;
    }

    void SptFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("SPT", frameNumber, mCache->getStats(), *stats);
    }
}
