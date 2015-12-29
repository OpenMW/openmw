#include "niffilemanager.hpp"

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

        NifFileHolder()
        {
        }

        META_Object(Resource, NifFileHolder)

        Nif::NIFFilePtr mNifFile;
    };

    NifFileManager::NifFileManager(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mCache = new osgDB::ObjectCache;
    }

    NifFileManager::~NifFileManager()
    {

    }

    void NifFileManager::clearCache()
    {
        // NIF files aren't needed any more when the converted objects are cached in SceneManager / BulletShapeManager,
        // so we'll simply drop all nif files here, unlikely to need them again
        mCache->clear();
    }

    Nif::NIFFilePtr NifFileManager::get(const std::string &name)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name);
        if (obj)
            return static_cast<NifFileHolder*>(obj.get())->mNifFile;
        else
        {
            Nif::NIFFilePtr file (new Nif::NIFFile(mVFS->getNormalized(name), name));
            obj = new NifFileHolder(file);
            mCache->addEntryToObjectCache(name, obj);
            return file;
        }
    }

}
