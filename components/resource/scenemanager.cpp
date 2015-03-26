#include "scenemanager.hpp"

#include <osg/Node>

#include <components/nifosg/nifloader.hpp>
#include <components/nif/niffile.hpp>

#include <components/sceneutil/clone.hpp>

namespace Resource
{

    SceneManager::SceneManager(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
    }

    osg::ref_ptr<const osg::Node> SceneManager::getTemplate(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        Index::iterator it = mIndex.find(normalized);
        if (it == mIndex.end())
        {
            Files::IStreamPtr file = mVFS->get(normalized);

            // TODO: add support for non-NIF formats

            NifOsg::Loader loader;
            loader.resourceManager = mVFS;
            osg::ref_ptr<const osg::Node> loaded = loader.load(Nif::NIFFilePtr(new Nif::NIFFile(file, normalized)));

            // TODO: provide way for the user to get textKeys (attach to the node?)

            mIndex[normalized] = loaded;
            return loaded;
        }
        else
            return it->second;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const std::string &name)
    {
        osg::ref_ptr<const osg::Node> scene = getTemplate(name);
        return osg::clone(scene.get(), SceneUtil::CopyOp());
    }

}
