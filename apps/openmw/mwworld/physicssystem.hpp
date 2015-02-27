#ifndef GAME_MWWORLD_PHYSICSSYSTEM_H
#define GAME_MWWORLD_PHYSICSSYSTEM_H

#include <memory>

#include <OgreVector3.h>

#include <btBulletCollisionCommon.h>

#include "ptr.hpp"


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

    typedef std::vector<std::pair<Ptr,Ogre::Vector3> > PtrVelocityList;

    class PhysicsSystem
    {
        public:
            PhysicsSystem (OEngine::Render::OgreRenderer &_rend);
            ~PhysicsSystem ();

            void enableWater(float height);
            void setWaterHeight(float height);
            void disableWater();

            void addObject (const MWWorld::Ptr& ptr, const std::string& mesh, bool placeable=false);

            void addActor (const MWWorld::Ptr& ptr, const std::string& mesh);

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

            void stepSimulation(float dt);

            std::vector<std::string> getCollisions(const MWWorld::Ptr &ptr, int collisionGroup, int collisionMask); ///< get handles this object collides with
            Ogre::Vector3 traceDown(const MWWorld::Ptr &ptr, float maxHeight);

            std::pair<float, std::string> getFacedHandle(float queryDistance);
            std::pair<std::string,Ogre::Vector3> getHitContact(const std::string &name,
                                                               const Ogre::Vector3 &origin,
                                                               const Ogre::Quaternion &orientation,
                                                               float queryDistance);
            std::vector < std::pair <float, std::string> > getFacedHandles (float queryDistance);
            std::vector < std::pair <float, std::string> > getFacedHandles (float mouseX, float mouseY, float queryDistance);

            // cast ray, return true if it hit something. if raycasringObjectOnlt is set to false, it ignores NPCs and objects with no collisions.
            bool castRay(const Ogre::Vector3& from, const Ogre::Vector3& to, bool raycastingObjectOnly = true,bool ignoreHeightMap = false);

            std::pair<bool, Ogre::Vector3>
            castRay(const Ogre::Vector3 &orig, const Ogre::Vector3 &dir, float len);

            std::pair<bool, Ogre::Vector3> castRay(float mouseX, float mouseY, Ogre::Vector3* normal = NULL, std::string* hit = NULL);
            ///< cast ray from the mouse, return true if it hit something and the first result
            /// @param normal if non-NULL, the hit normal will be written there (if there is a hit)
            /// @param hit if non-NULL, the string handle of the hit object will be written there (if there is a hit)

            OEngine::Physic::PhysicEngine* getEngine();

            bool getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max);

            /// Queues velocity movement for a Ptr. If a Ptr is already queued, its velocity will
            /// be overwritten. Valid until the next call to applyQueuedMovement.
            void queueObjectMovement(const Ptr &ptr, const Ogre::Vector3 &velocity);

            /// Apply all queued movements, then clear the list.
            const PtrVelocityList& applyQueuedMovement(float dt);

            /// Clear the queued movements list without applying.
            void clearQueuedMovement();

            /// Return true if \a actor has been standing on \a object in this frame
            /// This will trigger whenever the object is directly below the actor.
            /// It doesn't matter if the actor is stationary or moving.
            bool isActorStandingOn(const MWWorld::Ptr& actor, const MWWorld::Ptr& object) const;

            /// Get the handle of all actors standing on \a object in this frame.
            void getActorsStandingOn(const MWWorld::Ptr& object, std::vector<std::string>& out) const;

            /// Return true if \a actor has collided with \a object in this frame.
            /// This will detect running into objects, but will not detect climbing stairs, stepping up a small object, etc.
            bool isActorCollidingWith(const MWWorld::Ptr& actor, const MWWorld::Ptr& object) const;

            /// Get the handle of all actors colliding with \a object in this frame.
            void getActorsCollidingWith(const MWWorld::Ptr& object, std::vector<std::string>& out) const;

        private:

            void updateWater();

            OEngine::Render::OgreRenderer &mRender;
            OEngine::Physic::PhysicEngine* mEngine;
            std::map<std::string, std::string> handleToMesh;

            // Tracks all movement collisions happening during a single frame. <actor handle, collided handle>
            // This will detect e.g. running against a vertical wall. It will not detect climbing up stairs,
            // stepping up small objects, etc.
            std::map<std::string, std::string> mCollisions;

            std::map<std::string, std::string> mStandingCollisions;

            PtrVelocityList mMovementQueue;
            PtrVelocityList mMovementResults;

            float mTimeAccum;

            float mWaterHeight;
            float mWaterEnabled;

            std::auto_ptr<btCollisionObject> mWaterCollisionObject;
            std::auto_ptr<btCollisionShape> mWaterCollisionShape;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };
}

#endif
