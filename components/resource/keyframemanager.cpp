#include <osgDB/Registry>

#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>
#include <components/nifosg/nifloader.hpp>
#include <components/misc/stringops.hpp>

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
            if (normalized.size() > 5 && Misc::StringUtils::ciEqual(normalized.substr(normalized.size() - 5, 5), ".osgb")) {
                Files::IStreamPtr file = mVFS->getNormalized(normalized);
                std::string ext = "osgb";
                osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
                if (!reader) {
                    std::stringstream errormsg;
                    errormsg << "Error loading " << normalized << ": no readerwriter for '" << ext << "' found" << std::endl;
                    throw std::runtime_error(errormsg.str());
                }
                osg::ref_ptr<osgDB::Options> options (new osgDB::Options);
                osgDB::ReaderWriter::ReadResult result = reader->readObject(*file, options);
                if (!result.success()) {
                    std::stringstream errormsg;
                    errormsg << "Error loading " << normalized << ": " << result.message() << " code " << result.status() << std::endl;
                    throw std::runtime_error(errormsg.str());
                }
                osg::Object* obj = result.getObject();
                NifOsg::KeyframeHolder* kfh = dynamic_cast<NifOsg::KeyframeHolder*>(obj);
                osg::ref_ptr<NifOsg::KeyframeHolder> loaded(kfh);

                mCache->addEntryToObjectCache(normalized, loaded);
                return loaded;
            }
            else {
                osg::ref_ptr<NifOsg::KeyframeHolder> loaded (new NifOsg::KeyframeHolder);
                NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), *loaded.get());

                mCache->addEntryToObjectCache(normalized, loaded);
                return loaded;
            }
        }
    }
}
