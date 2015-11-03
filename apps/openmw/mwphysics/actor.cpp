#include "actor.hpp"

#include <osg/PositionAttitudeTransform>

#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <components/nifbullet/bulletnifloader.hpp>

#include "../mwworld/class.hpp"

#include "convert.hpp"
#include "collisiontype.hpp"

namespace MWPhysics
{


Actor::Actor(const MWWorld::Ptr& ptr, osg::ref_ptr<NifBullet::BulletShapeInstance> shape, btCollisionWorld* world)
  : mCanWaterWalk(false), mWalkingOnWater(false)
  , mCollisionObject(0), mForce(0.f, 0.f, 0.f), mOnGround(false)
  , mInternalCollisionMode(true)
  , mExternalCollisionMode(true)
  , mCollisionWorld(world)
{
    mPtr = ptr;

    mHalfExtents = shape->mCollisionBoxHalfExtents;
    mMeshTranslation = shape->mCollisionBoxTranslate;

    // Use capsule shape only if base is square (nonuniform scaling apparently doesn't work on it)
    if (std::abs(mHalfExtents.x()-mHalfExtents.y())<mHalfExtents.x()*0.05 && mHalfExtents.z() >= mHalfExtents.x())
    {
        // Could also be btCapsuleShapeZ, but the movement solver seems to have issues with it (jumping on slopes doesn't work)
        mShape.reset(new btCylinderShapeZ(toBullet(mHalfExtents)));
    }
    else
        mShape.reset(new btBoxShape(toBullet(mHalfExtents)));

    mCollisionObject.reset(new btCollisionObject);
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(static_cast<PtrHolder*>(this));

    updateRotation();
    updateScale();
    // already called by updateScale()
    //updatePosition();

    updateCollisionMask();
}

Actor::~Actor()
{
    if (mCollisionObject.get())
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

void Actor::updateCollisionMask()
{
    mCollisionWorld->removeCollisionObject(mCollisionObject.get());
    int collisionMask = CollisionType_World | CollisionType_HeightMap;
    if (mExternalCollisionMode)
        collisionMask |= CollisionType_Actor | CollisionType_Projectile;
    if (mCanWaterWalk)
        collisionMask |= CollisionType_Water;
    mCollisionWorld->addCollisionObject(mCollisionObject.get(), CollisionType_Actor, collisionMask);
}

void Actor::updatePosition()
{
    osg::Vec3f position = mPtr.getRefData().getPosition().asVec3();

    btTransform tr = mCollisionObject->getWorldTransform();
    osg::Vec3f scaledTranslation = mRotation * osg::componentMultiply(mMeshTranslation, mScale);
    osg::Vec3f newPosition = scaledTranslation + position;

    tr.setOrigin(toBullet(newPosition));
    mCollisionObject->setWorldTransform(tr);
}

osg::Vec3f Actor::getPosition() const
{
    return toOsg(mCollisionObject->getWorldTransform().getOrigin());
}

void Actor::updateRotation ()
{
    btTransform tr = mCollisionObject->getWorldTransform();
    mRotation = mPtr.getRefData().getBaseNode()->getAttitude();
    tr.setRotation(toBullet(mRotation));
    mCollisionObject->setWorldTransform(tr);

    updatePosition();
}

void Actor::updateScale()
{
    float scale = mPtr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale,scale,scale);

    mPtr.getClass().adjustScale(mPtr, scaleVec, false);
    mScale = scaleVec;
    mShape->setLocalScaling(toBullet(mScale));

    scaleVec = osg::Vec3f(scale,scale,scale);
    mPtr.getClass().adjustScale(mPtr, scaleVec, true);
    mRenderingScale = scaleVec;

    updatePosition();
}

osg::Vec3f Actor::getHalfExtents() const
{
    return osg::componentMultiply(mHalfExtents, mScale);
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
