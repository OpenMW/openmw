#ifndef OPENMW_MWPHYSICS_PHYSICSSYSTEM_H
#define OPENMW_MWPHYSICS_PHYSICSSYSTEM_H

#include <array>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

#include <osg/Quat>
#include <osg/BoundingBox>
#include <osg/ref_ptr>
#include <osg/Timer>

#include "../mwworld/ptr.hpp"

#include "collisiontype.hpp"
#include "raycasting.hpp"

namespace osg
{
    class Group;
    class Object;
    class Stats;
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
class btVector3;

namespace MWPhysics
{
    class HeightField;
    class Object;
    class Actor;
    class PhysicsTaskScheduler;
    class Projectile;

    using ActorMap = std::map<MWWorld::ConstPtr, std::shared_ptr<Actor>>;

    struct ContactPoint
    {
        MWWorld::Ptr mObject;
        osg::Vec3f mPoint;
        osg::Vec3f mNormal;
    };

    struct LOSRequest
    {
        LOSRequest(const std::weak_ptr<Actor>& a1, const std::weak_ptr<Actor>& a2);
        std::array<std::weak_ptr<Actor>, 2> mActors;
        std::array<const Actor*, 2> mRawActors;
        bool mResult;
        bool mStale;
        int mAge;
    };
    bool operator==(const LOSRequest& lhs, const LOSRequest& rhs) noexcept;

    struct ActorFrameData
    {
        ActorFrameData(const std::shared_ptr<Actor>& actor, const MWWorld::Ptr standingOn, bool moveToWaterSurface, float slowFall, float waterlevel);
        void  updatePosition(btCollisionWorld* world);
        std::weak_ptr<Actor> mActor;
        Actor* mActorRaw;
        MWWorld::Ptr mStandingOn;
        bool mFlying;
        bool mSwimming;
        bool mWasOnGround;
        bool mWantJump;
        bool mDidJump;
        bool mFloatToSurface;
        bool mNeedLand;
        bool mWaterCollision;
        bool mSkipCollisionDetection;
        float mWaterlevel;
        float mSlowFall;
        float mOldHeight;
        float mFallHeight;
        osg::Vec3f mMovement;
        osg::Vec3f mPosition;
        ESM::Position mRefpos;
    };

    struct WorldFrameData
    {
        WorldFrameData();
        bool mIsInStorm;
        osg::Vec3f mStormDirection;
    };

    class PhysicsSystem : public RayCastingInterface
    {
        public:
            PhysicsSystem (Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> parentNode);
            virtual ~PhysicsSystem ();

            void setUnrefQueue(SceneUtil::UnrefQueue* unrefQueue);

            Resource::BulletShapeManager* getShapeManager();

            void enableWater(float height);
            void setWaterHeight(float height);
            void disableWater();

            void addObject (const MWWorld::Ptr& ptr, const std::string& mesh, int collisionType = CollisionType_World);
            void addActor (const MWWorld::Ptr& ptr, const std::string& mesh);

            int addProjectile(const MWWorld::Ptr& caster, const osg::Vec3f& position, const std::string& mesh, bool computeRadius);
            void setCaster(int projectileId, const MWWorld::Ptr& caster);
            void updateProjectile(const int projectileId, const osg::Vec3f &position) const;
            void removeProjectile(const int projectileId);

            void updatePtr (const MWWorld::Ptr& old, const MWWorld::Ptr& updated);

            Actor* getActor(const MWWorld::Ptr& ptr);
            const Actor* getActor(const MWWorld::ConstPtr& ptr) const;

            const Object* getObject(const MWWorld::ConstPtr& ptr) const;

            Projectile* getProjectile(int projectileId) const;

            // Object or Actor
            void remove (const MWWorld::Ptr& ptr);

            void updateScale (const MWWorld::Ptr& ptr);
            void updateRotation (const MWWorld::Ptr& ptr);
            void updatePosition (const MWWorld::Ptr& ptr);

            void addHeightField (const float* heights, int x, int y, float triSize, float sqrtVerts, float minH, float maxH, const osg::Object* holdObject);

            void removeHeightField (int x, int y);

            const HeightField* getHeightField(int x, int y) const;

            bool toggleCollisionMode();

            void stepSimulation();
            void debugDraw();

            std::vector<MWWorld::Ptr> getCollisions(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const; ///< get handles this object collides with
            std::vector<ContactPoint> getCollisionsPoints(const MWWorld::ConstPtr &ptr, int collisionGroup, int collisionMask) const;
            osg::Vec3f traceDown(const MWWorld::Ptr &ptr, const osg::Vec3f& position, float maxHeight);

            std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(const MWWorld::ConstPtr& actor,
                                                               const osg::Vec3f &origin,
                                                               const osg::Quat &orientation,
                                                               float queryDistance, std::vector<MWWorld::Ptr>& targets);


            /// Get distance from \a point to the collision shape of \a target. Uses a raycast to find where the
            /// target vector hits the collision shape and then calculates distance from the intersection point.
            /// This can be used to find out how much nearer we need to move to the target for a "getHitContact" to be successful.
            /// \note Only Actor targets are supported at the moment.
            float getHitDistance(const osg::Vec3f& point, const MWWorld::ConstPtr& target) const override;

            /// @param me Optional, a Ptr to ignore in the list of results. targets are actors to filter for, ignoring all other actors.
            RayCastingResult castRay(const osg::Vec3f &from, const osg::Vec3f &to, const MWWorld::ConstPtr& ignore = MWWorld::ConstPtr(),
                    std::vector<MWWorld::Ptr> targets = std::vector<MWWorld::Ptr>(),
                    int mask = CollisionType_World|CollisionType_HeightMap|CollisionType_Actor|CollisionType_Door, int group=0xff) const override;

            RayCastingResult castSphere(const osg::Vec3f& from, const osg::Vec3f& to, float radius) const override;

            /// Return true if actor1 can see actor2.
            bool getLineOfSight(const MWWorld::ConstPtr& actor1, const MWWorld::ConstPtr& actor2) const override;

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

            /// Get bounding box in world space of the given object.
            osg::BoundingBox getBoundingBox(const MWWorld::ConstPtr &object) const;

            /// Queues velocity movement for a Ptr. If a Ptr is already queued, its velocity will
            /// be overwritten. Valid until the next call to applyQueuedMovement.
            void queueObjectMovement(const MWWorld::Ptr &ptr, const osg::Vec3f &velocity);

            /// Apply all queued movements, then clear the list.
            const std::vector<MWWorld::Ptr>& applyQueuedMovement(float dt, bool skipSimulation, osg::Timer_t frameStart, unsigned int frameNumber, osg::Stats& stats);

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

            void reportStats(unsigned int frameNumber, osg::Stats& stats) const;
            void reportCollision(const btVector3& position, const btVector3& normal);

        private:

            void updateWater();

            std::vector<ActorFrameData> prepareFrameData(bool willSimulate);

            osg::ref_ptr<SceneUtil::UnrefQueue> mUnrefQueue;

            std::unique_ptr<btBroadphaseInterface> mBroadphase;
            std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;
            std::unique_ptr<btCollisionDispatcher> mDispatcher;
            std::unique_ptr<btCollisionWorld> mCollisionWorld;
            std::unique_ptr<PhysicsTaskScheduler> mTaskScheduler;

            std::unique_ptr<Resource::BulletShapeManager> mShapeManager;
            Resource::ResourceSystem* mResourceSystem;

            using ObjectMap = std::map<MWWorld::ConstPtr, std::shared_ptr<Object>>;
            ObjectMap mObjects;

            std::set<Object*> mAnimatedObjects; // stores pointers to elements in mObjects

            ActorMap mActors;

            using ProjectileMap = std::map<int, std::shared_ptr<Projectile>>;
            ProjectileMap mProjectiles;

            using HeightFieldMap = std::map<std::pair<int, int>, osg::ref_ptr<HeightField>>;
            HeightFieldMap mHeightFields;

            bool mDebugDrawEnabled;

            float mTimeAccum;

            unsigned int mProjectileId;

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
