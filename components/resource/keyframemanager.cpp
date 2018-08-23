#include "keyframemanager.hpp"
#include <iostream>

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{

    KeyframeManager::KeyframeManager(const VFS::Manager* vfs)
        : ResourceManager(vfs)
    {
    }

    KeyframeManager::~KeyframeManager()
    {
    }

    osg::ref_ptr<const NifOsg::KeyframeHolder> KeyframeManager::get(const std::string &name, bool useKfFile)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const NifOsg::KeyframeHolder>(static_cast<NifOsg::KeyframeHolder*>(obj.get()));
        else
        {
            osg::ref_ptr<NifOsg::KeyframeHolder> loaded (new NifOsg::KeyframeHolder);

            if (useKfFile)
                NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), *loaded.get());
            else
            {
                std::cout << "use mesh! " << name << " " << normalized << std::endl;
                NifOsg::Loader::load(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), NULL, *loaded.get());
                std::cout << loaded.get()->mTextKeys.size() << std::endl;
            }

            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }

    void KeyframeManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        stats->setAttribute(frameNumber, "Keyframe", mCache->getCacheSize());
    }



}
