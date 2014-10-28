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
            std::map<std::string, std::string> mSceneNodeToMesh;
            OEngine::Physic::PhysicEngine* mEngine;

            Ogre::SceneManager *mSceneMgr;

        public:

            PhysicsSystem(Ogre::SceneManager *sceneMgr = NULL);
            ~PhysicsSystem();

            static PhysicsSystem *instance();

            void setSceneManager(Ogre::SceneManager *sceneMgr);

            void addObject(const std::string &mesh,
                    const std::string &sceneNodeName, const std::string &referenceId, float scale,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
                    bool placeable=false);

            void removeObject(const std::string &referenceId);

            void moveObject(const std::string &referenceId,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation);

            void addHeightField(float* heights,
                    int x, int y, float yoffset, float triSize, float sqrtVerts);

            void removeHeightField(int x, int y);

            void toggleDebugRendering();

            std::pair<std::string, Ogre::Vector3> castRay(float mouseX,
                    float mouseY, Ogre::Vector3* normal, std::string* hit, Ogre::Camera *camera);

            std::string referenceToSceneNode(std::string referenceId);
            std::string sceneNodeToMesh(std::string sceneNodeName);

        private:

            void updateSelectionHighlight(std::string sceneNode, const Ogre::Vector3 &position);
    };
}

#endif // CSV_WORLD_PHYSICSSYSTEM_H
