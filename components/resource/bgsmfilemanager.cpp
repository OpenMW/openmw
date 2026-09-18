#include "bgsmfilemanager.hpp"

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{

    BgsmFileManager::BgsmFileManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
    {
    }

    Bgsm::MaterialFilePtr BgsmFileManager::get(VFS::Path::NormalizedView name)
    {
        if (Bgsm::MaterialFilePtr cached = mCache->getRefFromObjectCache(name))
            return cached;

        Bgsm::MaterialFilePtr file = Bgsm::parse(mVFS->get(name));
        mCache->addEntryToObjectCache(name.value(), file);
        return file;
    }

    void BgsmFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("BSShader Material", frameNumber, mCache->getStats(), *stats);
    }

}
