#include "sptfilemanager.hpp"

#include <osg/Object>

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{
    class SptFileHolder : public osg::Object
    {
    public:
        SptFileHolder(const Spt::SPTFilePtr& file)
            : mSptFile(file)
        {
        }
        SptFileHolder(const SptFileHolder& copy, const osg::CopyOp& copyop)
            : mSptFile(copy.mSptFile)
        {
        }

        SptFileHolder() = default;

        META_Object(Resource, SptFileHolder)

        Spt::SPTFilePtr mSptFile;
    };

    SptFileManager::SptFileManager(const VFS::Manager* vfs, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
    {
    }

    Spt::SPTFilePtr SptFileManager::get(VFS::Path::NormalizedView name)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<SptFileHolder*>(obj.get())->mSptFile;
        else
        {
            Spt::SPTFilePtr file = Spt::parse(mVFS->get(name));
            obj = new SptFileHolder(file);
            mCache->addEntryToObjectCache(name.value(), obj);
            return file;
        }
    }

    void SptFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("SPT", frameNumber, mCache->getStats(), *stats);
    }
}