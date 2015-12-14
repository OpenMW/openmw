#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>
#include <components/nifosg/nifloader.hpp>

#include "objectcache.hpp"

namespace Resource
{

    KeyframeManager::KeyframeManager(const VFS::Manager* vfs)
        : mCache(new osgDB::ObjectCache)
        , mVFS(vfs)
    {
    }

    KeyframeManager::~KeyframeManager()
    {
    }

    osg::ref_ptr<const NifOsg::KeyframeHolder> KeyframeManager::get(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const NifOsg::KeyframeHolder>(static_cast<NifOsg::KeyframeHolder*>(obj.get()));
        else
        {
            osg::ref_ptr<NifOsg::KeyframeHolder> loaded (new NifOsg::KeyframeHolder);
            NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), *loaded.get());

            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }



}
