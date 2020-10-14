#include "actor.hpp"

#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"

namespace MWPhysics
{


Actor::Actor(const MWWorld::Ptr& ptr, const Resource::BulletShape* shape, btCollisionWorld* world)
  : mCanWaterWalk(false), mWalkingOnWater(false)
  , mCollisionObject(nullptr), mMeshTranslation(shape->mCollisionBoxTranslate), mHalfExtents(shape->mCollisionBoxHalfExtents)
  , mForce(0.f, 0.f, 0.f), mOnGround(true), mOnSlope(false)
  , mInternalCollisionMode(true)
  , mExternalCollisionMode(true)
  , mCollisionWorld(world)
{
    mPtr = ptr;

    // We can not create actor without collisions - he will fall through the ground.
    // In this case we should autogenerate collision box based on mesh shape
    // (NPCs have bodyparts and use a different approach)
    if (!ptr.getClass().isNpc() && mHalfExtents.length2() == 0.f)
    {
        if (shape->mCollisionShape)
        {
            btTransform transform;
            transform.setIdentity();
            btVector3 min;
            btVector3 max;

            shape->mCollisionShape->getAabb(transform, min, max);
            mHalfExtents.x() = (max[0] - min[0])/2.f;
            mHalfExtents.y() = (max[1] - min[1])/2.f;
            mHalfExtents.z() = (max[2] - min[2])/2.f;

            mMeshTranslation = osg::Vec3f(0.f, 0.f, mHalfExtents.z());
        }

        if (mHalfExtents.length2() == 0.f)
            Log(Debug::Error) << "Error: Failed to calculate bounding box for actor \"" << ptr.getCellRef().getRefId() << "\".";
    }

    // Use capsule shape only if base is square (nonuniform scaling apparently doesn't work on it)
    if (std::abs(mHalfExtents.x()-mHalfExtents.y())<mHalfExtents.x()*0.05 && mHalfExtents.z() >= mHalfExtents.x())
    {
        mShape.reset(new btCapsuleShapeZ(mHalfExtents.x(), 2*mHalfExtents.z() - 2*mHalfExtents.x()));
        mRotationallyInvariant = true;
    }
    else
    {
        mShape.reset(new btBoxShape(Misc::Convert::toBullet(mHalfExtents)));
        mRotationallyInvariant = false;
    }

    mConvexShape = static_cast<btConvexShape*>(mShape.get());

    mCollisionObject.reset(new btCollisionObject);
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(static_cast<PtrHolder*>(this));

    updateRotation();
    updateScale();
    updatePosition();

    addCollisionMask(getCollisionMask());
    commitPositionChange();
}

Actor::~Actor()
{
    if (mCollisionObject)
        mCollisionWorld->removeCollisionObject(mCollisionObject.get());
}

void Actor::enableCollisionMode(bool collision)
{
    mInternalCollisionMode = collision;
}

void Actor::enableCollisionBody(bool collision)
{
    if (mExternalCollisionMode != collision)
    {
        mExternalCollisionMode = collision;
        updateCollisionMask();
    }
}

void Actor::addCollisionMask(int collisionMask)
{
    mCollisionWorld->addCollisionObject(mCollisionObject.get(), CollisionType_Actor, collisionMask);
}

void Actor::updateCollisionMask()
{
    mCollisionObject->getBroadphaseHandle()->m_collisionFilterMask = getCollisionMask();
}

int Actor::getCollisionMask() const
{
    int collisionMask = CollisionType_World | CollisionType_HeightMap;
    if (mExternalCollisionMode)
        collisionMask |= CollisionType_Actor | CollisionType_Projectile | CollisionType_Door;
    if (mCanWaterWalk)
        collisionMask |= CollisionType_Water;
    return collisionMask;
}

void Actor::updatePosition()
{
    osg::Vec3f position = mPtr.getRefData().getPosition().asVec3();

    mPosition = position;
    mPreviousPosition = position;

    mTransformUpdatePending = true;
    updateCollisionObjectPosition();
}

void Actor::updateCollisionObjectPosition()
{
    osg::Vec3f scaledTranslation = mRotation * osg::componentMultiply(mMeshTranslation, mScale);
    osg::Vec3f newPosition = scaledTranslation + mPosition;
    mLocalTransform.setOrigin(Misc::Convert::toBullet(newPosition));
    mLocalTransform.setRotation(Misc::Convert::toBullet(mRotation));

}

void Actor::commitPositionChange()
{
    if (mScaleUpdatePending)
    {
        mShape->setLocalScaling(Misc::Convert::toBullet(mScale));
        mScaleUpdatePending = false;
    }
    if (mTransformUpdatePending)
    {
        mCollisionObject->setWorldTransform(mLocalTransform);
        mTransformUpdatePending = false;
    }
}

osg::Vec3f Actor::getCollisionObjectPosition() const
{
    return Misc::Convert::toOsg(mLocalTransform.getOrigin());
}

void Actor::setPosition(const osg::Vec3f &position)
{
    if (mTransformUpdatePending)
    {
        mCollisionObject->setWorldTransform(mLocalTransform);
        mTransformUpdatePending = false;
    }
    else
    {
        mPreviousPosition = mPosition;

        mPosition = position;
        updateCollisionObjectPosition();
        mCollisionObject->setWorldTransform(mLocalTransform);
    }
}

osg::Vec3f Actor::getPosition() const
{
    return mPosition;
}

osg::Vec3f Actor::getPreviousPosition() const
{
    return mPreviousPosition;
}

void Actor::updateRotation ()
{
    if (mRotation == mPtr.getRefData().getBaseNode()->getAttitude())
        return;
    mRotation = mPtr.getRefData().getBaseNode()->getAttitude();

    mTransformUpdatePending = true;
    updateCollisionObjectPosition();
}

bool Actor::isRotationallyInvariant() const
{
    return mRotationallyInvariant;
}

void Actor::updateScale()
{
    float scale = mPtr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale,scale,scale);

    mPtr.getClass().adjustScale(mPtr, scaleVec, false);
    mScale = scaleVec;
    mScaleUpdatePending = true;

    scaleVec = osg::Vec3f(scale,scale,scale);
    mPtr.getClass().adjustScale(mPtr, scaleVec, true);
    mRenderingScale = scaleVec;

    mTransformUpdatePending = true;
    updateCollisionObjectPosition();
}

osg::Vec3f Actor::getHalfExtents() const
{
    return osg::componentMultiply(mHalfExtents, mScale);
}

osg::Vec3f Actor::getOriginalHalfExtents() const
{
    return mHalfExtents;
}

osg::Vec3f Actor::getRenderingHalfExtents() const
{
    return osg::componentMultiply(mHalfExtents, mRenderingScale);
}

void Actor::setInertialForce(const osg::Vec3f &force)
{
    mForce = force;
}

void Actor::setOnGround(bool grounded)
{
    mOnGround = grounded;
}

void Actor::setOnSlope(bool slope)
{
    mOnSlope = slope;
}

bool Actor::isWalkingOnWater() const
{
    return mWalkingOnWater;
}

void Actor::setWalkingOnWater(bool walkingOnWater)
{
    mWalkingOnWater = walkingOnWater;
}

void Actor::setCanWaterWalk(bool waterWalk)
{
    if (waterWalk != mCanWaterWalk)
    {
        mCanWaterWalk = waterWalk;
        updateCollisionMask();
    }
}

}
