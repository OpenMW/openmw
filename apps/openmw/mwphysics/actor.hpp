#ifndef OPENMW_MWPHYSICS_ACTOR_H
#define OPENMW_MWPHYSICS_ACTOR_H

#include <memory>

#include "../mwworld/ptr.hpp"

#include <osg/Vec3f>
#include <osg/Quat>
#include <osg/ref_ptr>

class btCollisionWorld;
class btCollisionShape;
class btCollisionObject;

namespace NifBullet
{
    class BulletShapeInstance;
}

namespace MWPhysics
{

    class PtrHolder
    {
    public:
        virtual ~PtrHolder() {}

        void updatePtr(const MWWorld::Ptr& updated)
        {
            mPtr = updated;
        }

        MWWorld::Ptr getPtr() const
        {
            return mPtr;
        }

    protected:
        MWWorld::Ptr mPtr;
    };

    class Actor : public PtrHolder
    {
    public:
        Actor(const MWWorld::Ptr& ptr, osg::ref_ptr<NifBullet::BulletShapeInstance> shape, btCollisionWorld* world);
        ~Actor();

        /**
         * Sets the collisionMode for this actor. If disabled, the actor can fly and clip geometry.
         */
        void enableCollisionMode(bool collision);

        bool getCollisionMode() const
        {
            return mInternalCollisionMode;
        }

        /**
         * Enables or disables the *external* collision body. If disabled, other actors will not collide with this actor.
         */
        void enableCollisionBody(bool collision);

        void updateScale();
        void updateRotation();
        void updatePosition();

        /**
         * Returns the (scaled) half extents
         */
        osg::Vec3f getHalfExtents() const;

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

        bool mCanWaterWalk;
        bool mWalkingOnWater;

        std::unique_ptr<btCollisionShape> mShape;

        std::unique_ptr<btCollisionObject> mCollisionObject;

        osg::Vec3f mMeshTranslation;
        osg::Vec3f mHalfExtents;
        osg::Quat mRotation;

        osg::Vec3f mScale;
        osg::Vec3f mPosition;

        osg::Vec3f mForce;
        bool mOnGround;
        bool mInternalCollisionMode;
        bool mExternalCollisionMode;

        btCollisionWorld* mCollisionWorld;

        Actor(const Actor&);
        Actor& operator=(const Actor&);
    };

}


#endif
