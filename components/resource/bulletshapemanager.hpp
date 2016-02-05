#ifndef OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H
#define OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H

#include <map>
#include <string>

#include <osg/ref_ptr>

#include "bulletshape.hpp"

namespace VFS
{
    class Manager;
}

namespace osgDB
{
    class ObjectCache;
}

namespace Resource
{
    class SceneManager;
    class NifFileManager;

    class BulletShape;
    class BulletShapeInstance;

    class BulletShapeManager
    {
    public:
        BulletShapeManager(const VFS::Manager* vfs, SceneManager* sceneMgr, NifFileManager* nifFileManager);
        ~BulletShapeManager();

        osg::ref_ptr<BulletShapeInstance> createInstance(const std::string& name);

    private:
        const VFS::Manager* mVFS;
        SceneManager* mSceneManager;
        NifFileManager* mNifFileManager;

        osg::ref_ptr<osgDB::ObjectCache> mCache;
    };

}

#endif
