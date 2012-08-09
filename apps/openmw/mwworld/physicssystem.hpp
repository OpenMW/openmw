#ifndef GAME_MWWORLD_PHYSICSSYSTEM_H
#define GAME_MWWORLD_PHYSICSSYSTEM_H

#include <openengine/ogre/renderer.hpp>
#include "ptr.hpp"
#include <openengine/bullet/pmove.h>

namespace MWWorld
{

    class PhysicsSystem
    {
        public:
            PhysicsSystem (OEngine::Render::OgreRenderer &_rend);
            ~PhysicsSystem ();

            void doPhysics(float duration, const std::vector<std::pair<std::string, Ogre::Vector3> >& actors);
            ///< do physics with dt - Usage: first call doPhysics with frame dt, then call doPhysicsFixed as often as time steps have passed

            std::vector< std::pair<std::string, Ogre::Vector3> > doPhysicsFixed (const std::vector<std::pair<std::string, Ogre::Vector3> >& actors);
            ///< do physics with fixed timestep - Usage: first call doPhysics with frame dt, then call doPhysicsFixed as often as time steps have passed

            void addObject (const std::string& handle, const std::string& mesh,
                const Ogre::Quaternion& rotation, float scale, const Ogre::Vector3& position);

            void addActor (const std::string& handle, const std::string& mesh,
                const Ogre::Vector3& position);

            void addHeightField (float* heights,
                int x, int y, float yoffset,
                float triSize, float sqrtVerts);

            void removeHeightField (int x, int y);

            void removeObject (const std::string& handle);

            void moveObject (const std::string& handle, const Ogre::Vector3& position);

            void rotateObject (const std::string& handle, const Ogre::Quaternion& rotation);

            void scaleObject (const std::string& handle, float scale);

            bool toggleCollisionMode();
            
            std::pair<std::string, float> getFacedHandle (MWWorld::World& world);

            btVector3 getRayPoint(float extent);
            btVector3 getRayPoint(float extent, float mouseX, float mouseY);

            std::vector < std::pair <float, std::string> > getFacedObjects ();

            std::vector < std::pair <float, std::string> > getFacedObjects (float mouseX, float mouseY);

            // cast ray, return true if it hit something
            bool castRay(const Ogre::Vector3& from, const Ogre::Vector3& to);

            std::pair<bool, Ogre::Vector3>
            castRay(const Ogre::Vector3 &orig, const Ogre::Vector3 &dir, float len);

            std::pair<bool, Ogre::Vector3> castRay(float mouseX, float mouseY);
            ///< cast ray from the mouse, return true if it hit something and the first result (in OGRE coordinates)

            void insertObjectPhysics(const MWWorld::Ptr& ptr, std::string model);

            void insertActorPhysics(const MWWorld::Ptr&, std::string model);

            OEngine::Physic::PhysicEngine* getEngine();

            void setCurrentWater(bool hasWater, int waterHeight);

            bool getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max);

        private:
            OEngine::Render::OgreRenderer &mRender;
            OEngine::Physic::PhysicEngine* mEngine;
            bool mFreeFly;
            playerMove* playerphysics;
            std::map<std::string, std::string> handleToMesh;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };

}

#endif
