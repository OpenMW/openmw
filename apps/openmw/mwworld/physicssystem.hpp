#ifndef GAME_MWWORLD_PHYSICSSYSTEM_H
#define GAME_MWWORLD_PHYSICSSYSTEM_H

#include <vector>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/physic.hpp>
#include "ptr.hpp"
#include <openengine/bullet/pmove.h>

namespace MWWorld
{

    class PhysicsSystem
    {
        public:
            PhysicsSystem (OEngine::Render::OgreRenderer &_rend);
            ~PhysicsSystem ();

            std::vector< std::pair<std::string, Ogre::Vector3> > doPhysics (float duration,
                const std::vector<std::pair<std::string, Ogre::Vector3> >& actors);

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

            std::vector < std::pair <float, std::string> > getFacedObjects ();

            // cast ray, return true if it hit something
            bool castRay(const Ogre::Vector3& from, const Ogre::Vector3& to);

            void insertObjectPhysics(const MWWorld::Ptr& ptr, std::string model);

            void insertActorPhysics(const MWWorld::Ptr&, std::string model);

            OEngine::Physic::PhysicEngine* getEngine();
            
            void setCurrentWater(bool hasWater, int waterHeight);

        private:
            OEngine::Render::OgreRenderer &mRender;
            OEngine::Physic::PhysicEngine* mEngine;
            bool mFreeFly;
            playerMove* playerphysics;


            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };

}

#endif
