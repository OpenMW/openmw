#ifndef OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H
#define OPENMW_COMPONENTS_BULLETSHAPEMANAGER_H

#include <map>
#include <string>

#include <osg/ref_ptr>

namespace VFS
{
    class Manager;
}

namespace Resource
{
    class SceneManager;
}

namespace NifBullet
{

    class BulletShape;
    class BulletShapeInstance;

    class BulletShapeManager
    {
    public:
        BulletShapeManager(const VFS::Manager* vfs, Resource::SceneManager* sceneManager);
        ~BulletShapeManager();

        osg::ref_ptr<BulletShapeInstance> createInstance(const std::string& name);

    private:
        const VFS::Manager* mVFS;
        // need to load keyframes to know what nodes are going to be animated
        Resource::SceneManager* mSceneManager;

        typedef std::map<std::string, osg::ref_ptr<BulletShape> > Index;
        Index mIndex;
    };

}

#endif
