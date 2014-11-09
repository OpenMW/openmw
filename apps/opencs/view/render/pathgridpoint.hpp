#ifndef OPENCS_VIEW_PATHGRIDPOINT_H
#define OPENCS_VIEW_PATHGRIDPOINT_H

#include <components/nifogre/ogrenifloader.hpp>

namespace Ogre
{
    class Vector3;
    class SceneNode;
    class SceneManager;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSVRender
{
    class PathgridPoint
    {
            CSVWorld::PhysicsSystem *mPhysics; // local copy
            NifOgre::ObjectScenePtr mPgPoint;
            Ogre::SceneNode *mBase;

        public:

            PathgridPoint(const std::string &name,
                Ogre::SceneNode *cellNode, const Ogre::Vector3 &pos, CSVWorld::PhysicsSystem *physics);

            ~PathgridPoint();

            static std::pair<std::string, int> getIdAndIndex(const std::string &name);

            static std::string getName(const std::string &pathgridId, int index);
    };
}

#endif // OPENCS_VIEW_PATHGRIDPOINT_H
