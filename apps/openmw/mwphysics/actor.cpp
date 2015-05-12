#include "actor.hpp"
#include <osg/io_utils>

#include <osg/PositionAttitudeTransform>

#include <btBulletCollisionCommon.h>

#include "../mwworld/class.hpp"

#include <components/nifbullet/bulletnifloader.hpp>

#include "convert.hpp"
#include "collisiontype.hpp"

namespace MWPhysics
{


Actor::Actor(const MWWorld::Ptr& ptr, osg::ref_ptr<NifBullet::BulletShapeInstance> shape, btDynamicsWorld* world)
  : mCanWaterWalk(false), mWalkingOnWater(false)
  , mCollisionObject(0), mForce(0.f, 0.f, 0.f), mOnGround(false)
  , mInternalCollisionMode(true)
  , mExternalCollisionMode(true)
  , mDynamicsWorld(world)
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
        mDynamicsWorld->removeCollisionObject(mCollisionObject.get());
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
    mDynamicsWorld->removeCollisionObject(mCollisionObject.get());
    int collisionMask = CollisionType_World | CollisionType_HeightMap;
    if (mExternalCollisionMode)
        collisionMask |= CollisionType_Actor | CollisionType_Projectile;
    if (mCanWaterWalk)
        collisionMask |= CollisionType_Water;
    mDynamicsWorld->addCollisionObject(mCollisionObject.get(), CollisionType_Actor, collisionMask);
}

void Actor::updatePosition()
{
    osg::Vec3f position = mPtr.getRefData().getPosition().asVec3();

    btTransform tr = mCollisionObject->getWorldTransform();
    osg::Vec3f scaledTranslation = osg::componentMultiply(mMeshTranslation, mScale);
    osg::Vec3f newPosition = scaledTranslation + position;

    tr.setOrigin(toBullet(newPosition));
    mCollisionObject->setWorldTransform(tr);
}

void Actor::updateRotation ()
{
    btTransform tr = mCollisionObject->getWorldTransform();
    tr.setRotation(toBullet(mPtr.getRefData().getBaseNode()->getAttitude()));
    mCollisionObject->setWorldTransform(tr);
}

void Actor::updateScale()
{
    float scale = mPtr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale,scale,scale);

    if (!mPtr.getClass().isNpc())
        mPtr.getClass().adjustScale(mPtr, scaleVec);

    mScale = scaleVec;
    mShape->setLocalScaling(toBullet(mScale));

    updatePosition();
}

osg::Vec3f Actor::getHalfExtents() const
{
    return osg::componentMultiply(mHalfExtents, mScale);
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
