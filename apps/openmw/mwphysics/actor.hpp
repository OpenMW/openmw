#ifndef OPENMW_MWPHYSICS_ACTOR_H
#define OPENMW_MWPHYSICS_ACTOR_H

#include <memory>

#include "ptrholder.hpp"

#include <osg/Vec3f>
#include <osg/Quat>
#include <osg/ref_ptr>

class btCollisionWorld;
class btCollisionShape;
class btCollisionObject;
class btConvexShape;

namespace Resource
{
    class BulletShape;
}

namespace MWPhysics
{

    class Actor : public PtrHolder
    {
    public:
        Actor(const MWWorld::Ptr& ptr, osg::ref_ptr<const Resource::BulletShape> shape, btCollisionWorld* world);
        ~Actor();

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
        void updateRotation();

        /**
         * Return true if the collision shape looks the same no matter how its Z rotated.
         */
        bool isRotationallyInvariant() const;

        /**
         * Set mPosition and mPreviousPosition to the position in the Ptr's RefData. This should be used
         * when an object is "instantly" moved/teleported as opposed to being moved by the physics simulation.
         */
        void updatePosition();

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
          */
        void setPosition(const osg::Vec3f& position);

        osg::Vec3f getPosition() const;

        osg::Vec3f getPreviousPosition() const;

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

        btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
        }

        /// Sets whether this actor should be able to collide with the water surface
        void setCanWaterWalk(bool waterWalk);

        /// Sets whether this actor has been walking on the water surface in the last frame
        void setWalkingOnWater(bool walkingOnWater);
        bool isWalkingOnWater() const;

    private:
        /// Removes then re-adds the collision object to the dynamics world
        void updateCollisionMask();
        void addCollisionMask(int collisionMask);
        int getCollisionMask();

        bool mCanWaterWalk;
        bool mWalkingOnWater;

        bool mRotationallyInvariant;

        std::unique_ptr<btCollisionShape> mShape;
        btConvexShape* mConvexShape;

        std::unique_ptr<btCollisionObject> mCollisionObject;

        osg::Vec3f mMeshTranslation;
        osg::Vec3f mHalfExtents;
        osg::Quat mRotation;

        osg::Vec3f mScale;
        osg::Vec3f mRenderingScale;
        osg::Vec3f mPosition;
        osg::Vec3f mPreviousPosition;

        osg::Vec3f mForce;
        bool mOnGround;
        bool mOnSlope;
        bool mInternalCollisionMode;
        bool mExternalCollisionMode;

        btCollisionWorld* mCollisionWorld;

        Actor(const Actor&);
        Actor& operator=(const Actor&);
    };

}


#endif
