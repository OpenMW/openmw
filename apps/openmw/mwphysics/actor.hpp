#ifndef OPENMW_MWPHYSICS_ACTOR_H
#define OPENMW_MWPHYSICS_ACTOR_H

#include <memory>
#include <mutex>

#include "ptrholder.hpp"

#include <LinearMath/btTransform.h>
#include <osg/Vec3f>
#include <osg/Quat>

class btCollisionShape;
class btCollisionObject;
class btCollisionWorld;
class btConvexShape;

namespace Resource
{
    class BulletShape;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler;

    class Actor final : public PtrHolder
    {
    public:
        Actor(const MWWorld::Ptr& ptr, const Resource::BulletShape* shape, PhysicsTaskScheduler* scheduler, bool canWaterWalk);
        ~Actor() override;

        /**
         * Sets the collisionMode for this actor. If disabled, the actor can fly and clip geometry.
         */
        void enableCollisionMode(bool collision);

        bool getCollisionMode() const
        {
            return mInternalCollisionMode;
        }

        btConvexShape* getConvexShape() const { return mConvexShape; }

        /**
         * Enables or disables the *external* collision body. If disabled, other actors will not collide with this actor.
         */
        void enableCollisionBody(bool collision);

        void updateScale();
        void setRotation(osg::Quat quat);

        /**
         * Return true if the collision shape looks the same no matter how its Z rotated.
         */
        bool isRotationallyInvariant() const;

        /**
        * Used by the physics simulation to store the simulation result. Used in conjunction with mWorldPosition
        * to account for e.g. scripted movements
        */
        void setSimulationPosition(const osg::Vec3f& position);

        void updateCollisionObjectPosition();

        /**
         * Returns the half extents of the collision body (scaled according to collision scale)
         */
        osg::Vec3f getHalfExtents() const;

        /**
         * Returns the half extents of the collision body (not scaled)
         */
        osg::Vec3f getOriginalHalfExtents() const;

        /**
         * Returns the position of the collision body
         * @note The collision shape's origin is in its center, so the position returned can be described as center of the actor collision box in world space.
         */
        osg::Vec3f getCollisionObjectPosition() const;

        /**
          * Store the current position into mPreviousPosition, then move to this position.
          * Returns true if the new position is different.
          */
        bool setPosition(const osg::Vec3f& position);

        // force set actor position to be as in Ptr::RefData
        void updatePosition();

        // register a position offset that will be applied during simulation.
        void adjustPosition(const osg::Vec3f& offset, bool ignoreCollisions);

        // apply position offset. Can't be called during simulation
        void applyOffsetChange();

        /**
         * Returns the half extents of the collision body (scaled according to rendering scale)
         * @note The reason we need this extra method is because of an inconsistency in MW - NPC race scales aren't applied to the collision shape,
         * most likely to make environment collision testing easier. However in some cases (swimming level) we want the actual scale.
         */
        osg::Vec3f getRenderingHalfExtents() const;

        /**
         * Sets the current amount of inertial force (incl. gravity) affecting this physic actor
         */
        void setInertialForce(const osg::Vec3f &force);

        /**
         * Gets the current amount of inertial force (incl. gravity) affecting this physic actor
         */
        const osg::Vec3f &getInertialForce() const
        {
            return mForce;
        }

        void setOnGround(bool grounded);

        bool getOnGround() const
        {
            return mInternalCollisionMode && mOnGround;
        }

        void setOnSlope(bool slope);

        bool getOnSlope() const
        {
            return mInternalCollisionMode && mOnSlope;
        }

        /// Sets whether this actor should be able to collide with the water surface
        void setCanWaterWalk(bool waterWalk);

        /// Sets whether this actor has been walking on the water surface in the last frame
        void setWalkingOnWater(bool walkingOnWater);
        bool isWalkingOnWater() const;

        MWWorld::Ptr getStandingOnPtr() const;
        void setStandingOnPtr(const MWWorld::Ptr& ptr);

        unsigned int getStuckFrames() const
        {
            return mStuckFrames;
        }
        void setStuckFrames(unsigned int frames)
        {
            mStuckFrames = frames;
        }

        const osg::Vec3f &getLastStuckPosition() const
        {
            return mLastStuckPosition;
        }
        void setLastStuckPosition(osg::Vec3f position)
        {
            mLastStuckPosition = position;
        }

        bool skipCollisions();

        bool canMoveToWaterSurface(float waterlevel, const btCollisionWorld* world) const;

    private:
        MWWorld::Ptr mStandingOnPtr;
        /// Removes then re-adds the collision object to the dynamics world
        void updateCollisionMask();
        void addCollisionMask(int collisionMask);
        int getCollisionMask() const;

        /// Returns the mesh translation, scaled and rotated as necessary
        osg::Vec3f getScaledMeshTranslation() const;

        bool mCanWaterWalk;
        bool mWalkingOnWater;

        bool mRotationallyInvariant;

        std::unique_ptr<btCollisionShape> mShape;
        btConvexShape* mConvexShape;

        osg::Vec3f mMeshTranslation;
        osg::Vec3f mOriginalHalfExtents;
        osg::Vec3f mHalfExtents;
        osg::Vec3f mRenderingHalfExtents;
        osg::Quat mRotation;

        osg::Vec3f mScale;
        osg::Vec3f mPositionOffset;
        bool mWorldPositionChanged;
        bool mSkipCollisions;
        bool mSkipSimulation;
        mutable std::mutex mPositionMutex;

        unsigned int mStuckFrames;
        osg::Vec3f mLastStuckPosition;

        osg::Vec3f mForce;
        bool mOnGround;
        bool mOnSlope;
        bool mInternalCollisionMode;
        bool mExternalCollisionMode;

        PhysicsTaskScheduler* mTaskScheduler;

        Actor(const Actor&);
        Actor& operator=(const Actor&);
    };

}


#endif
