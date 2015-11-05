#ifndef OPENMW_MWPHYSICS_PHYSICSSYSTEM_H
#define OPENMW_MWPHYSICS_PHYSICSSYSTEM_H

#include <memory>
#include <map>

#include <osg/Quat>
#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

#include "collisiontype.hpp"

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
            const Actor* getActor(const MWWorld::Ptr& ptr) const;

            // Object or Actor
            void remove (const MWWorld::Ptr& ptr);

            void updateScale (const MWWorld::Ptr& ptr);
            void updateRotation (const MWWorld::Ptr& ptr);
            void updatePosition (const MWWorld::Ptr& ptr);


            void addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts);

            void removeHeightField (int x, int y);

            bool toggleCollisionMode();

            void stepSimulation(float dt);
            void debugDraw();

            std::vector<MWWorld::Ptr> getCollisions(const MWWorld::Ptr &ptr, int collisionGroup, int collisionMask); ///< get handles this object collides with
            osg::Vec3f traceDown(const MWWorld::Ptr &ptr, float maxHeight);

            std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(const MWWorld::Ptr& actor,
                                                               const osg::Vec3f &origin,
                                                               const osg::Quat &orientation,
                                                               float queryDistance);

            struct RayResult
            {
                bool mHit;
                osg::Vec3f mHitPos;
                osg::Vec3f mHitNormal;
                MWWorld::Ptr mHitObject;
            };

            /// @param me Optional, a Ptr to ignore in the list of results
            RayResult castRay(const osg::Vec3f &from, const osg::Vec3f &to, MWWorld::Ptr ignore = MWWorld::Ptr(), int mask =
                    CollisionType_World|CollisionType_HeightMap|CollisionType_Actor, int group=0xff);

            RayResult castSphere(const osg::Vec3f& from, const osg::Vec3f& to, float radius);

            /// Return true if actor1 can see actor2.
            bool getLineOfSight(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2);

            bool isOnGround (const MWWorld::Ptr& actor);

            /// Get physical half extents (scaled) of the given actor.
            osg::Vec3f getHalfExtents(const MWWorld::Ptr& actor) const;

            /// @see MWPhysics::Actor::getRenderingHalfExtents
            osg::Vec3f getRenderingHalfExtents(const MWWorld::Ptr& actor) const;

            /// Get the position of the collision shape for the actor. Use together with getHalfExtents() to get the collision bounds in world space.
            /// @note The collision shape's origin is in its center, so the position returned can be described as center of the actor collision box in world space.
            osg::Vec3f getPosition(const MWWorld::Ptr& actor) const;

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
