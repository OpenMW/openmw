#ifndef CSV_WORLD_PHYSICSSYSTEM_H
#define CSV_WORLD_PHYSICSSYSTEM_H

#include <map>
#include <vector>

namespace Ogre
{
    class Vector3;
    class Quaternion;
    class SceneManager;
    class Camera;
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
            Ogre::SceneManager *mSceneMgr;
            std::map<std::string, std::vector<std::string>> mSelectedEntities;

        public:

            PhysicsSystem(Ogre::SceneManager *sceneMgr);
            ~PhysicsSystem();

            void addObject(const std::string &mesh,
                           const std::string &name,
                           float scale,
                           const Ogre::Vector3 &position,
                           const Ogre::Quaternion &rotation,
                           //Ogre::Vector3* scaledBoxTranslation = 0,
                           //Ogre::Quaternion* boxRotation = 0,
                           //bool raycasting=false,
                           bool placeable=false);

            void toggleDebugRendering();

            /*std::pair<bool, Ogre::Vector3>*/ void castRay(float mouseX, float mouseY,
                                                   Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera);
    };
}

#endif // CSV_WORLD_PHYSICSSYSTEM_H
