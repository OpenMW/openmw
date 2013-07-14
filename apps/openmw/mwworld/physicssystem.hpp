#ifndef GAME_MWWORLD_PHYSICSSYSTEM_H
#define GAME_MWWORLD_PHYSICSSYSTEM_H

#include <OgreVector3.h>

#include <btBulletCollisionCommon.h>


namespace OEngine
{
    namespace Render
    {
        class OgreRenderer;
    }
    namespace Physic
    {
        class PhysicEngine;
    }
}

namespace MWWorld
{
    class World;
    class Ptr;

    class PhysicsSystem
    {
        public:
            PhysicsSystem (OEngine::Render::OgreRenderer &_rend);
            ~PhysicsSystem ();

            void addObject (const MWWorld::Ptr& ptr, bool placeable=false);

            void addActor (const MWWorld::Ptr& ptr);

            void addHeightField (float* heights,
                int x, int y, float yoffset,
                float triSize, float sqrtVerts);

            void removeHeightField (int x, int y);

            // have to keep this as handle for now as unloadcell only knows scenenode names
            void removeObject (const std::string& handle);

            void moveObject (const MWWorld::Ptr& ptr);

            void rotateObject (const MWWorld::Ptr& ptr);

            void scaleObject (const MWWorld::Ptr& ptr);

            bool toggleCollisionMode();
            
            Ogre::Vector3 move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time, bool gravity);
            std::vector<std::string> getCollisions(const MWWorld::Ptr &ptr); ///< get handles this object collides with
            Ogre::Vector3 traceDown(const MWWorld::Ptr &ptr);

            std::pair<float, std::string> getFacedHandle (MWWorld::World& world, float queryDistance);
            std::vector < std::pair <float, std::string> > getFacedHandles (float queryDistance);
            std::vector < std::pair <float, std::string> > getFacedHandles (float mouseX, float mouseY, float queryDistance);

            btVector3 getRayPoint(float extent);
            btVector3 getRayPoint(float extent, float mouseX, float mouseY);


            // cast ray, return true if it hit something. if raycasringObjectOnlt is set to false, it ignores NPCs and objects with no collisions.
            bool castRay(const Ogre::Vector3& from, const Ogre::Vector3& to, bool raycastingObjectOnly = true,bool ignoreHeightMap = false);

            std::pair<bool, Ogre::Vector3>
            castRay(const Ogre::Vector3 &orig, const Ogre::Vector3 &dir, float len);

            std::pair<bool, Ogre::Vector3> castRay(float mouseX, float mouseY);
            ///< cast ray from the mouse, return true if it hit something and the first result (in OGRE coordinates)

            OEngine::Physic::PhysicEngine* getEngine();

            void setCurrentWater(bool hasWater, int waterHeight);

            bool getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max);

            void updateCameraData(const Ogre::Vector3 &eyepos, float pitch, float yaw);

        private:
            struct {
                Ogre::Vector3 eyepos;
                float pitch, yaw;
            } mCameraData;

            OEngine::Render::OgreRenderer &mRender;
            OEngine::Physic::PhysicEngine* mEngine;
            std::map<std::string, std::string> handleToMesh;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };
}

#endif
