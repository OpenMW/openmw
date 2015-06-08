#include "bulletshapemanager.hpp"

#include <components/vfs/manager.hpp>

#include <components/nifbullet/bulletnifloader.hpp>

#include <components/resource/scenemanager.hpp>

namespace NifBullet
{

BulletShapeManager::BulletShapeManager(const VFS::Manager* vfs, Resource::SceneManager* sceneManager)
    : mVFS(vfs)
    , mSceneManager(sceneManager)
{

}

BulletShapeManager::~BulletShapeManager()
{

}

osg::ref_ptr<BulletShapeInstance> BulletShapeManager::createInstance(const std::string &name)
{
    std::string normalized = name;
    mVFS->normalizeFilename(normalized);

    osg::ref_ptr<BulletShape> shape;
    Index::iterator it = mIndex.find(normalized);
    if (it == mIndex.end())
    {
        Files::IStreamPtr file = mVFS->get(normalized);

        // TODO: add support for non-NIF formats

        std::string kfname = normalized;
        if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
            kfname.replace(kfname.size()-4, 4, ".kf");
        std::set<std::string> animatedNodes;
        if (mVFS->exists(kfname))
        {
            osg::ref_ptr<const NifOsg::KeyframeHolder> keyframes = mSceneManager->getKeyframes(kfname);
            for (NifOsg::KeyframeHolder::KeyframeControllerMap::const_iterator it = keyframes->mKeyframeControllers.begin();
                 it != keyframes->mKeyframeControllers.end(); ++it)
                animatedNodes.insert(it->first);
        }

        BulletNifLoader loader;
        loader.setAnimatedNodes(animatedNodes);
        // might be worth sharing NIFFiles with SceneManager in some way
        shape = loader.load(Nif::NIFFilePtr(new Nif::NIFFile(file, normalized)));

        mIndex[normalized] = shape;
    }
    else
        shape = it->second;

    osg::ref_ptr<BulletShapeInstance> instance = shape->makeInstance();
    return instance;
}

}
