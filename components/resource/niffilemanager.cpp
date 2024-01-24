#include "niffilemanager.hpp"

#include <iostream>

#include <osg/Object>
#include <osg/Stats>

#include <components/vfs/manager.hpp>

#include "objectcache.hpp"

namespace Resource
{

    class NifFileHolder : public osg::Object
    {
    public:
        NifFileHolder(const Nif::NIFFilePtr& file)
            : mNifFile(file)
        {
        }
        NifFileHolder(const NifFileHolder& copy, const osg::CopyOp& copyop)
            : mNifFile(copy.mNifFile)
        {
        }

        NifFileHolder() = default;

        META_Object(Resource, NifFileHolder)

        Nif::NIFFilePtr mNifFile;
    };

    NifFileManager::NifFileManager(const VFS::Manager* vfs, const ToUTF8::StatelessUtf8Encoder* encoder)
        // NIF files aren't needed any more once the converted objects are cached in SceneManager / BulletShapeManager,
        // so no point in using an expiry delay.
        : ResourceManager(vfs, 0)
        , mEncoder(encoder)
    {
    }

    NifFileManager::~NifFileManager() = default;

    Nif::NIFFilePtr NifFileManager::get(const std::string& name)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<NifFileHolder*>(obj.get())->mNifFile;
        else
        {
            auto file = std::make_shared<Nif::NIFFile>(name);
            Nif::Reader reader(*file, mEncoder);
            reader.parse(mVFS->get(name));
            obj = new NifFileHolder(file);
            mCache->addEntryToObjectCache(name, obj);
            return file;
        }
    }

    void NifFileManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Nif", mCache->getCacheSize());
    }

}
