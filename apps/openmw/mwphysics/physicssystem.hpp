#ifndef OPENMW_MWPHYSICS_PHYSICSSYSTEM_H
#define OPENMW_MWPHYSICS_PHYSICSSYSTEM_H

#include <memory>

#include <OgreVector3.h>

#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

#include <osg/Quat>

namespace osg
{
    class Group;
}

namespace MWRender
{
    class DebugDrawer;
}

namespace NifBullet
{
    class BulletShapeManager;
}

namespace Resource
{
    class ResourceSystem;
}

class btCollisionWorld;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btCollisionObject;
class btCollisionShape;

namespace MWPhysics
{
    typedef std::vector<std::pair<MWWorld::Ptr,osg::Vec3f> > PtrVelocityList;

    class HeightField;
    class Object;
    class Actor;

    class PhysicsSystem
    {
        public:
            PhysicsSystem (Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode);
            ~PhysicsSystem ();

            void enableWater(float height);
            void setWaterHeight(float height);
            void disableWater();

            void addObject (const MWWorld::Ptr& ptr, const std::string& mesh);
            void addActor (const MWWorld::Ptr& ptr, const std::string& mesh);

            void updatePtr (const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

            Actor* getActor(const MWWorld::Ptr& ptr);

            // Object or Actor
            void remove (const MWWorld::Ptr& ptr);

            void updateScale (const MWWorld::Ptr& ptr);
            void updateRotation (const MWWorld::Ptr& ptr);
            void updatePosition (const MWWorld::Ptr& ptr);


            void addHeightField (float* heights, int x, int y, float triSize, float sqrtVerts);

            void removeHeightField (int x, int y);

            bool toggleCollisionMode();

            void stepSimulation(float dt);

            std::vector<MWWorld::Ptr> getCollisions(const MWWorld::Ptr &ptr, int collisionGroup, int collisionMask); ///< get handles this object collides with
            osg::Vec3f traceDown(const MWWorld::Ptr &ptr, float maxHeight);

            std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(const MWWorld::Ptr& actor,
                                                               const osg::Vec3f &origin,
                                                               const osg::Quat &orientation,
                                                               float queryDistance);

            // cast ray, return true if it hit something.
            bool castRay(const Ogre::Vector3& from, const Ogre::Vector3& to,bool ignoreHeightMap = false);

            /// @return <bool hit, world hit position>
            std::pair<bool, osg::Vec3f> castRay(const osg::Vec3f &from, const osg::Vec3f &to);

            /// Queues velocity movement for a Ptr. If a Ptr is already queued, its velocity will
            /// be overwritten. Valid until the next call to applyQueuedMovement.
            void queueObjectMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &velocity);

            /// Apply all queued movements, then clear the list.
            const PtrVelocityList& applyQueuedMovement(float dt);

            /// Clear the queued movements list without applying.
            void clearQueuedMovement();

            /// Return true if \a actor has been standing on \a object in this frame
            /// This will trigger whenever the object is directly below the actor.
            /// It doesn't matter if the actor is stationary or moving.
            bool isActorStandingOn(const MWWorld::Ptr& actor, const MWWorld::Ptr& object) const;

            /// Get the handle of all actors standing on \a object in this frame.
            void getActorsStandingOn(const MWWorld::Ptr& object, std::vector<MWWorld::Ptr>& out) const;

            /// Return true if \a actor has collided with \a object in this frame.
            /// This will detect running into objects, but will not detect climbing stairs, stepping up a small object, etc.
            bool isActorCollidingWith(const MWWorld::Ptr& actor, const MWWorld::Ptr& object) const;

            /// Get the handle of all actors colliding with \a object in this frame.
            void getActorsCollidingWith(const MWWorld::Ptr& object, std::vector<MWWorld::Ptr>& out) const;

            bool toggleDebugRendering();

        private:

            void updateWater();

            btBroadphaseInterface* mBroadphase;
            btDefaultCollisionConfiguration* mCollisionConfiguration;
            btCollisionDispatcher* mDispatcher;
            btCollisionWorld* mCollisionWorld;

            std::auto_ptr<NifBullet::BulletShapeManager> mShapeManager;

            typedef std::map<MWWorld::Ptr, Object*> ObjectMap;
            ObjectMap mObjects;

            typedef std::map<MWWorld::Ptr, Actor*> ActorMap;
            ActorMap mActors;

            typedef std::map<std::pair<int, int>, HeightField*> HeightFieldMap;
            HeightFieldMap mHeightFields;

            bool mDebugDrawEnabled;

            // Tracks all movement collisions happening during a single frame. <actor handle, collided handle>
            // This will detect e.g. running against a vertical wall. It will not detect climbing up stairs,
            // stepping up small objects, etc.
            typedef std::map<MWWorld::Ptr, MWWorld::Ptr> CollisionMap;
            CollisionMap mCollisions;
            CollisionMap mStandingCollisions;

            // replaces all occurences of 'old' in the map by 'updated', no matter if its a key or value
            void updateCollisionMapPtr(CollisionMap& map, const MWWorld::Ptr &old, const MWWorld::Ptr &updated);

            PtrVelocityList mMovementQueue;
            PtrVelocityList mMovementResults;

            float mTimeAccum;

            float mWaterHeight;
            float mWaterEnabled;

            std::auto_ptr<btCollisionObject> mWaterCollisionObject;
            std::auto_ptr<btCollisionShape> mWaterCollisionShape;

            std::auto_ptr<MWRender::DebugDrawer> mDebugDrawer;

            osg::ref_ptr<osg::Group> mParentNode;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };
}

#endif
