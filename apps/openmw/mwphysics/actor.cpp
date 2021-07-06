#include "actor.hpp"

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
#include "mtphysics.hpp"

#include <cmath>

namespace MWPhysics
{


Actor::Actor(const MWWorld::Ptr& ptr, const Resource::BulletShape* shape, PhysicsTaskScheduler* scheduler)
  : mStandingOnPtr(nullptr), mCanWaterWalk(false), mWalkingOnWater(false)
  , mCollisionObject(nullptr), mMeshTranslation(shape->mCollisionBox.center), mHalfExtents(shape->mCollisionBox.extents)
  , mVelocity(0,0,0), mStuckFrames(0), mLastStuckPosition{0, 0, 0}
  , mForce(0.f, 0.f, 0.f), mOnGround(true), mOnSlope(false)
  , mInternalCollisionMode(true)
  , mExternalCollisionMode(true)
  , mTaskScheduler(scheduler)
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

    mShape.reset(new btBoxShape(Misc::Convert::toBullet(mHalfExtents)));
    mRotationallyInvariant = (mMeshTranslation.x() == 0.0 && mMeshTranslation.y() == 0.0) && std::fabs(mHalfExtents.x() - mHalfExtents.y()) < 2.2;

    mConvexShape = static_cast<btConvexShape*>(mShape.get());

    mCollisionObject = std::make_unique<btCollisionObject>();
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(this);

    updateScale();

    if(!mRotationallyInvariant)
        setRotation(mPtr.getRefData().getBaseNode()->getAttitude());

    updatePosition();
    addCollisionMask(getCollisionMask());
    updateCollisionObjectPosition();
}

Actor::~Actor()
{
    mTaskScheduler->removeCollisionObject(mCollisionObject.get());
}

void Actor::enableCollisionMode(bool collision)
{
    mInternalCollisionMode.store(collision, std::memory_order_release);
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
    mTaskScheduler->addCollisionObject(mCollisionObject.get(), CollisionType_Actor, collisionMask);
}

void Actor::updateCollisionMask()
{
    mTaskScheduler->setCollisionFilterMask(mCollisionObject.get(), getCollisionMask());
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
    std::scoped_lock lock(mPositionMutex);
    const auto worldPosition = mPtr.getRefData().getPosition().asVec3();
    mPreviousPosition = worldPosition;
    mPosition = worldPosition;
    mSimulationPosition = worldPosition;
    mPositionOffset = osg::Vec3f();
    mStandingOnPtr = nullptr;
    mSkipCollisions = true;
}

void Actor::setSimulationPosition(const osg::Vec3f& position)
{
    mSimulationPosition = position;
}

osg::Vec3f Actor::getSimulationPosition() const
{
    return mSimulationPosition;
}

osg::Vec3f Actor::getScaledMeshTranslation() const
{
    return mRotation * osg::componentMultiply(mMeshTranslation, mScale);
}

void Actor::updateCollisionObjectPosition()
{
    std::scoped_lock lock(mPositionMutex);
    mShape->setLocalScaling(Misc::Convert::toBullet(mScale));
    osg::Vec3f scaledTranslation = mRotation * osg::componentMultiply(mMeshTranslation, mScale);
    osg::Vec3f newPosition = scaledTranslation + mPosition;
    mLocalTransform.setOrigin(Misc::Convert::toBullet(newPosition));
    mLocalTransform.setRotation(Misc::Convert::toBullet(mRotation));
    mCollisionObject->setWorldTransform(mLocalTransform);
    mWorldPositionChanged = false;
}

osg::Vec3f Actor::getCollisionObjectPosition() const
{
    std::scoped_lock lock(mPositionMutex);
    return Misc::Convert::toOsg(mLocalTransform.getOrigin());
}

bool Actor::setPosition(const osg::Vec3f& position)
{
    std::scoped_lock lock(mPositionMutex);
    applyOffsetChange();
    bool hasChanged = mPosition != position || mWorldPositionChanged;
    mPreviousPosition = mPosition;
    mPosition = position;
    return hasChanged;
}

void Actor::adjustPosition(const osg::Vec3f& offset, bool ignoreCollisions)
{
    std::scoped_lock lock(mPositionMutex);
    mPositionOffset += offset;
    mSkipCollisions = mSkipCollisions || ignoreCollisions;
}

void Actor::applyOffsetChange()
{
    if (mPositionOffset.length() == 0)
        return;
    mPosition += mPositionOffset;
    mPreviousPosition += mPositionOffset;
    mSimulationPosition += mPositionOffset;
    mPositionOffset = osg::Vec3f();
    mWorldPositionChanged = true;
}

osg::Vec3f Actor::getPosition() const
{
    return mPosition;
}

osg::Vec3f Actor::getPreviousPosition() const
{
    return mPreviousPosition;
}

void Actor::setRotation(osg::Quat quat)
{
    std::scoped_lock lock(mPositionMutex);
    mRotation = quat;
}

bool Actor::isRotationallyInvariant() const
{
    return mRotationallyInvariant;
}

void Actor::updateScale()
{
    std::scoped_lock lock(mPositionMutex);
    float scale = mPtr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale,scale,scale);

    mPtr.getClass().adjustScale(mPtr, scaleVec, false);
    mScale = scaleVec;

    scaleVec = osg::Vec3f(scale,scale,scale);
    mPtr.getClass().adjustScale(mPtr, scaleVec, true);
    mRenderingScale = scaleVec;
}

osg::Vec3f Actor::getHalfExtents() const
{
    std::scoped_lock lock(mPositionMutex);
    return osg::componentMultiply(mHalfExtents, mScale);
}

osg::Vec3f Actor::getOriginalHalfExtents() const
{
    return mHalfExtents;
}

osg::Vec3f Actor::getRenderingHalfExtents() const
{
    std::scoped_lock lock(mPositionMutex);
    return osg::componentMultiply(mHalfExtents, mRenderingScale);
}

void Actor::setInertialForce(const osg::Vec3f &force)
{
    mForce = force;
}

void Actor::setOnGround(bool grounded)
{
    mOnGround.store(grounded, std::memory_order_release);
}

void Actor::setOnSlope(bool slope)
{
    mOnSlope.store(slope, std::memory_order_release);
}

bool Actor::isWalkingOnWater() const
{
    return mWalkingOnWater.load(std::memory_order_acquire);
}

void Actor::setWalkingOnWater(bool walkingOnWater)
{
    mWalkingOnWater.store(walkingOnWater, std::memory_order_release);
}

void Actor::setCanWaterWalk(bool waterWalk)
{
    if (waterWalk != mCanWaterWalk)
    {
        mCanWaterWalk = waterWalk;
        updateCollisionMask();
    }
}

MWWorld::Ptr Actor::getStandingOnPtr() const
{
    std::scoped_lock lock(mPositionMutex);
    return mStandingOnPtr;
}

void Actor::setStandingOnPtr(const MWWorld::Ptr& ptr)
{
    std::scoped_lock lock(mPositionMutex);
    mStandingOnPtr = ptr;
}

bool Actor::skipCollisions()
{
    return std::exchange(mSkipCollisions, false);
}

void Actor::setVelocity(osg::Vec3f velocity)
{
    mVelocity = velocity;
}

osg::Vec3f Actor::velocity()
{
    return std::exchange(mVelocity, osg::Vec3f());
}

}
