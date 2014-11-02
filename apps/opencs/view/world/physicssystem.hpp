#ifndef CSV_WORLD_PHYSICSSYSTEM_H
#define CSV_WORLD_PHYSICSSYSTEM_H

#include <string>
#include <map>

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

namespace CSVRender
{
    class SceneWidget;
}

namespace CSVWorld
{
    class PhysicsSystem
    {
            std::map<std::string, std::string> mSceneNodeToRefId;
            std::map<std::string, std::map<Ogre::SceneManager *, std::string> > mRefIdToSceneNode;
            std::map<std::string, std::string> mSceneNodeToMesh;
            std::map<Ogre::SceneManager*, CSVRender::SceneWidget *> mSceneWidgets;
            OEngine::Physic::PhysicEngine* mEngine;
            std::multimap<std::string, Ogre::SceneManager *> mTerrain;

        public:

            PhysicsSystem();
            ~PhysicsSystem();

            void addSceneManager(Ogre::SceneManager *sceneMgr, CSVRender::SceneWidget * scene);

            void removeSceneManager(Ogre::SceneManager *sceneMgr);

            void addObject(const std::string &mesh,
                    const std::string &sceneNodeName, const std::string &referenceId, float scale,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
                    bool placeable=false);

            void removeObject(const std::string &sceneNodeName);
            void removePhysicsObject(const std::string &sceneNodeName);

            void replaceObject(const std::string &sceneNodeName,
                    float scale, const Ogre::Vector3 &position,
                    const Ogre::Quaternion &rotation, bool placeable=false);

            void moveObject(const std::string &sceneNodeName,
                    const Ogre::Vector3 &position, const Ogre::Quaternion &rotation);

            void moveSceneNodes(const std::string sceneNodeName, const Ogre::Vector3 &position);

            void addHeightField(Ogre::SceneManager *sceneManager,
                    float* heights, int x, int y, float yoffset, float triSize, float sqrtVerts);

            void removeHeightField(Ogre::SceneManager *sceneManager, int x, int y);

            void toggleDebugRendering(Ogre::SceneManager *sceneMgr);

            // return the object's SceneNode name and position for the given SceneManager
            std::pair<std::string, Ogre::Vector3> castRay(float mouseX,
                    float mouseY, Ogre::SceneManager *sceneMgr, Ogre::Camera *camera);

            std::string sceneNodeToRefId(std::string sceneNodeName);

            // for multi-scene manager per physics engine
            std::map<Ogre::SceneManager*, CSVRender::SceneWidget *> sceneWidgets();

        private:

            void moveSceneNodeImpl(const std::string sceneNodeName,
                    const std::string referenceId, const Ogre::Vector3 &position);

            void updateSelectionHighlight(std::string sceneNode, const Ogre::Vector3 &position);

            std::string refIdToSceneNode(std::string referenceId, Ogre::SceneManager *sceneMgr);

            Ogre::SceneManager *findSceneManager(std::string sceneNodeName);
    };
}

#endif // CSV_WORLD_PHYSICSSYSTEM_H
