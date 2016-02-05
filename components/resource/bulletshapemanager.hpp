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

    /// Handles loading, caching and "instancing" of bullet shapes.
    /// A shape 'instance' is a clone of another shape, with the goal of setting a different scale on this instance.
    /// @note May be used from any thread.
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
