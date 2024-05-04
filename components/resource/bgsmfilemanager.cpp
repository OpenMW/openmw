#include "bgsmfilemanager.hpp"

#include <osg/Object>

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{

    class BgsmFileHolder : public osg::Object
    {
    public:
        BgsmFileHolder(const Bgsm::MaterialFilePtr& file)
            : mBgsmFile(file)
        {
        }
        BgsmFileHolder(const BgsmFileHolder& copy, const osg::CopyOp& copyop)
            : mBgsmFile(copy.mBgsmFile)
        {
        }

        BgsmFileHolder() = default;

        META_Object(Resource, BgsmFileHolder)

        Bgsm::MaterialFilePtr mBgsmFile;
    };

    BgsmFileManager::BgsmFileManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
    {
    }

    Bgsm::MaterialFilePtr BgsmFileManager::get(VFS::Path::NormalizedView name)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<BgsmFileHolder*>(obj.get())->mBgsmFile;
        else
        {
            Bgsm::MaterialFilePtr file = Bgsm::parse(mVFS->get(name));
            obj = new BgsmFileHolder(file);
            mCache->addEntryToObjectCache(name.value(), obj);
            return file;
        }
    }

    void BgsmFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("BSShader Material", frameNumber, mCache->getStats(), *stats);
    }

}
