#ifndef OPENMW_MWPHYSICS_PHYSICSSYSTEM_H
#define OPENMW_MWPHYSICS_PHYSICSSYSTEM_H

#include <memory>
#include <map>
#include <set>
#include <algorithm>

#include <osg/Quat>
#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

#include "collisiontype.hpp"

namespace osg
{
    class Group;
    class Object;
}

namespace MWRender
{
    class DebugDrawer;
}

namespace Resource
{
    class BulletShapeManager;
    class ResourceSystem;
}

namespace SceneUtil
{
    class UnrefQueue;
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

            void setUnrefQueue(SceneUtil::UnrefQueue* unrefQueue);

            Resource::BulletShapeManager* getShapeManager();

            void enableWater(float height);
            void setWaterHeight(float height);
            void disableWater();

            void addObject (const MWWorld::Ptr& ptr, const std::string& mesh, int collisionType = CollisionType_World);
            void addActor (const MWWorld::Ptr& ptr, const std::string& mesh);

            void updatePtr (const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

            Actor* getActor(const MWWorld::Ptr& ptr);
            const Actor* getActor(const MWWorld::ConstPtr& ptr) const;

            const Object* getObject(const MWWorld::ConstPtr& ptr) const;

            // Object or Actor
            void remove (const MWWorld::Ptr& ptr);

            void updateScale (const MWWorld::Ptr& ptr);
            void updateRotation (const MWWorld::Ptr& ptr);
            void updatePosition (const MWWorld::Ptr& ptr);


            void addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject);

            void removeHeightField (int x, int y);

            const HeightField* getHeightField(int x, int y) const;

            bool toggleCollisionMode();

            void stepSimulation(float dt);
            void debugDraw();

            std::vector<MWWorld::Ptr> getCollisions(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const; ///< get handles this object collides with
            osg::Vec3f traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, float maxHeight);

            std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(const MWWorld::ConstPtr& actor,
                                                               const osg::Vec3f &origin,
                                                               const osg::Quat &orientation,
                                                               float queryDistance, std::vector<MWWorld::Ptr> targets = std::vector<MWWorld::Ptr>());


            /// Get distance from \a point to the collision shape of \a target. Uses a raycast to find where the
            /// target vector hits the collision shape and then calculates distance from the intersection point.
            /// This can be used to find out how much nearer we need to move to the target for a "getHitContact" to be successful.
            /// \note Only Actor targets are supported at the moment.
            float getHitDistance(const osg::Vec3f& point, const MWWorld::ConstPtr& target) const;

            struct RayResult
            {
                bool mHit;
                osg::Vec3f mHitPos;
                osg::Vec3f mHitNormal;
                MWWorld::Ptr mHitObject;
            };

            /// @param me Optional, a Ptr to ignore in the list of results. targets are actors to filter for, ignoring all other actors.
            RayResult castRay(const osg::Vec3f &from, const osg::Vec3f &to, const MWWorld::ConstPtr& ignore = MWWorld::ConstPtr(),
                    std::vector<MWWorld::Ptr> targets = std::vector<MWWorld::Ptr>(),
                    int mask = CollisionType_World|CollisionType_HeightMap|CollisionType_Actor|CollisionType_Door, int group=0xff) const;

            RayResult castSphere(const osg::Vec3f& from, const osg::Vec3f& to, float radius);

            /// Return true if actor1 can see actor2.
            bool getLineOfSight(const MWWorld::ConstPtr& actor1, const MWWorld::ConstPtr& actor2) const;

            bool isOnGround (const MWWorld::Ptr& actor);

            bool canMoveToWaterSurface (const MWWorld::ConstPtr &actor, const float waterlevel);

            /// Get physical half extents (scaled) of the given actor.
            osg::Vec3f getHalfExtents(const MWWorld::ConstPtr& actor) const;

            /// Get physical half extents (not scaled) of the given actor.
            osg::Vec3f getOriginalHalfExtents(const MWWorld::ConstPtr& actor) const;

            /// @see MWPhysics::Actor::getRenderingHalfExtents
            osg::Vec3f getRenderingHalfExtents(const MWWorld::ConstPtr& actor) const;

            /// Get the position of the collision shape for the actor. Use together with getHalfExtents() to get the collision bounds in world space.
            /// @note The collision shape's origin is in its center, so the position returned can be described as center of the actor collision box in world space.
            osg::Vec3f getCollisionObjectPosition(const MWWorld::ConstPtr& actor) const;

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
            bool isActorStandingOn(const MWWorld::Ptr& actor, const MWWorld::ConstPtr& object) const;

            /// Get the handle of all actors standing on \a object in this frame.
            void getActorsStandingOn(const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr>& out) const;

            /// Return true if \a actor has collided with \a object in this frame.
            /// This will detect running into objects, but will not detect climbing stairs, stepping up a small object, etc.
            bool isActorCollidingWith(const MWWorld::Ptr& actor, const MWWorld::ConstPtr& object) const;

            /// Get the handle of all actors colliding with \a object in this frame.
            void getActorsCollidingWith(const MWWorld::ConstPtr& object, std::vector<MWWorld::Ptr>& out) const;

            bool toggleDebugRendering();

            /// Mark the given object as a 'non-solid' object. A non-solid object means that
            /// \a isOnSolidGround will return false for actors standing on that object.
            void markAsNonSolid (const MWWorld::ConstPtr& ptr);

            bool isOnSolidGround (const MWWorld::Ptr& actor) const;

            void updateAnimatedCollisionShape(const MWWorld::Ptr& object);

            template <class Function>
            void forEachAnimatedObject(Function&& function) const
            {
                std::for_each(mAnimatedObjects.begin(), mAnimatedObjects.end(), function);
            }

            bool isAreaOccupiedByOtherActor(const osg::Vec3f& position, const float radius, const MWWorld::ConstPtr& ignore) const;

        private:

            void updateWater();

            osg::ref_ptr<SceneUtil::UnrefQueue> mUnrefQueue;

            btBroadphaseInterface* mBroadphase;
            btDefaultCollisionConfiguration* mCollisionConfiguration;
            btCollisionDispatcher* mDispatcher;
            btCollisionWorld* mCollisionWorld;

            std::unique_ptr<Resource::BulletShapeManager> mShapeManager;
            Resource::ResourceSystem* mResourceSystem;

            typedef std::map<MWWorld::ConstPtr, Object*> ObjectMap;
            ObjectMap mObjects;

            std::set<Object*> mAnimatedObjects; // stores pointers to elements in mObjects

            typedef std::map<MWWorld::ConstPtr, Actor*> ActorMap;
            ActorMap mActors;

            typedef std::map<std::pair<int, int>, HeightField*> HeightFieldMap;
            HeightFieldMap mHeightFields;

            bool mDebugDrawEnabled;

            // Tracks standing collisions happening during a single frame. <actor handle, collided handle>
            // This will detect standing on an object, but won't detect running e.g. against a wall.
            typedef std::map<MWWorld::Ptr, MWWorld::Ptr> CollisionMap;
            CollisionMap mStandingCollisions;

            // replaces all occurrences of 'old' in the map by 'updated', no matter if it's a key or value
            void updateCollisionMapPtr(CollisionMap& map, const MWWorld::Ptr &old, const MWWorld::Ptr &updated);

            PtrVelocityList mMovementQueue;
            PtrVelocityList mMovementResults;

            float mTimeAccum;

            float mWaterHeight;
            bool mWaterEnabled;

            std::unique_ptr<btCollisionObject> mWaterCollisionObject;
            std::unique_ptr<btCollisionShape> mWaterCollisionShape;

            std::unique_ptr<MWRender::DebugDrawer> mDebugDrawer;

            osg::ref_ptr<osg::Group> mParentNode;

            float mPhysicsDt;

            PhysicsSystem (const PhysicsSystem&);
            PhysicsSystem& operator= (const PhysicsSystem&);
    };
}

#endif
