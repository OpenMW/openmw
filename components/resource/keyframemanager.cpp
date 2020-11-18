#include "keyframemanager.hpp"

#include <components/vfs/manager.hpp>

#include <components/nifosg/nifloader.hpp>

#include "objectcache.hpp"
#include "scenemanager.hpp"

namespace
{
    class RetrieveAnimationsVisitor : public osg::NodeVisitor
    {
    public:
        RetrieveAnimationsVisitor(SceneUtil::KeyframeHolder& target) : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN), mTarget(target) {}


        virtual void apply(osg::Node& node)
        {
            if (node.libraryName() == std::string("osgAnimation"))
                std::cout << "found an " << node.className() << std::endl;
            traverse(node);
        }

    private:
        SceneUtil::KeyframeHolder& mTarget;
    };

    std::string getFileExtension(const std::string& file)
    {
        size_t extPos = file.find_last_of('.');
        if (extPos != std::string::npos && extPos+1 < file.size())
            return file.substr(extPos+1);
        return std::string();
    }
}

namespace Resource
{

    KeyframeManager::KeyframeManager(const VFS::Manager* vfs, SceneManager* sceneManager)
        : ResourceManager(vfs)
        , mSceneManager(sceneManager)
    {
    }

    KeyframeManager::~KeyframeManager()
    {
    }

    osg::ref_ptr<const SceneUtil::KeyframeHolder> KeyframeManager::get(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const SceneUtil::KeyframeHolder>(static_cast<SceneUtil::KeyframeHolder*>(obj.get()));
        else
        {
            osg::ref_ptr<SceneUtil::KeyframeHolder> loaded (new SceneUtil::KeyframeHolder);
            std::string ext = getFileExtension(normalized);
            if (ext == "kf")
            {
                NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(mVFS->getNormalized(normalized), normalized)), *loaded.get());
            }
            else
            {
                osg::ref_ptr<const osg::Node> scene = mSceneManager->getTemplate(normalized);
                RetrieveAnimationsVisitor rav(*loaded.get());
                const_cast<osg::Node*>(scene.get())->accept(rav); // const_cast required because there is no const version of osg::NodeVisitor
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
