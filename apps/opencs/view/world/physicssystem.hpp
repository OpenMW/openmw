#ifndef CSV_WORLD_PHYSICSSYSTEM_H
#define CSV_WORLD_PHYSICSSYSTEM_H

#include <map>

namespace Ogre
{
    class Vector3;
    class Quaternion;
    class SceneManager;
}

namespace OEngine
{
    namespace Physic
    {
        class PhysicEngine;
    }
}

namespace CSVWorld
{
    class PhysicsSystem
    {
            std::map<std::string, std::string> handleToMesh;
            OEngine::Physic::PhysicEngine* mEngine;
            //Ogre::SceneManager *mSceneMgr;

        public:

            PhysicsSystem(Ogre::SceneManager *sceneMgr);
            ~PhysicsSystem();

            void addObject(const std::string &mesh,
                           const std::string &name,
                           float scale,
                           const Ogre::Vector3 &position,
                           const Ogre::Quaternion &rotation,
                           Ogre::Vector3* scaledBoxTranslation = 0,
                           Ogre::Quaternion* boxRotation = 0,
                           bool raycasting=false,
                           bool placeable=false);

            void toggleDebugRendering();
    };
}

#endif // CSV_WORLD_PHYSICSSYSTEM_H
