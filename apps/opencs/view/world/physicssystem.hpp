#ifndef CSV_WORLD_PHYSICSSYSTEM_H
#define CSV_WORLD_PHYSICSSYSTEM_H

#include <string>
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
            static PhysicsSystem *mPhysicsSystemInstance;
            std::map<std::string, std::string> mRefToSceneNode;
            OEngine::Physic::PhysicEngine* mEngine;

            Ogre::SceneManager *mSceneMgr;
            std::map<std::string, std::vector<std::string> > mSelectedEntities;

        public:

            PhysicsSystem(Ogre::SceneManager *sceneMgr = NULL);
            ~PhysicsSystem();

            static PhysicsSystem *instance();

            void setSceneManager(Ogre::SceneManager *sceneMgr);

            void addObject(const std::string &mesh, const std::string &name,
                            const std::string &referenceId, float scale,
                            const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
                            bool placeable=false);

            void removeObject(const std::string &name);

            void addHeightField(float* heights, int x, int y, float yoffset,
                            float triSize, float sqrtVerts);

            void removeHeightField(int x, int y);

            void toggleDebugRendering();

            std::pair<std::string, Ogre::Vector3> castRay(float mouseX, float mouseY,
                            Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera);

        private:

            void initDebug();
            void updateSelectionHighlight(std::string sceneNode, const Ogre::Vector3 &position);
    };
}

#endif // CSV_WORLD_PHYSICSSYSTEM_H
