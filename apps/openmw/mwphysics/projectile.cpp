#include <memory>

#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <LinearMath/btVector3.h>

#include <components/misc/convert.hpp>

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"
#include "memory"
#include "mtphysics.hpp"
#include "projectile.hpp"

namespace MWPhysics
{
Projectile::Projectile(const MWWorld::Ptr& caster, const osg::Vec3f& position, float radius, bool canCrossWaterSurface, PhysicsTaskScheduler* scheduler, PhysicsSystem* physicssystem)
    : mCanCrossWaterSurface(canCrossWaterSurface)
    , mCrossedWaterSurface(false)
    , mActive(true)
    , mCaster(caster)
    , mWaterHitPosition(std::nullopt)
    , mPhysics(physicssystem)
    , mTaskScheduler(scheduler)
{
    mShape = std::make_unique<btSphereShape>(radius);
    mConvexShape = static_cast<btConvexShape*>(mShape.get());

    mCollisionObject = std::make_unique<btCollisionObject>();
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(this);

    setPosition(position);

    const int collisionMask = CollisionType_World | CollisionType_HeightMap |
        CollisionType_Actor | CollisionType_Door | CollisionType_Water | CollisionType_Projectile;
    mTaskScheduler->addCollisionObject(mCollisionObject.get(), CollisionType_Projectile, collisionMask);

    commitPositionChange();
}

Projectile::~Projectile()
{
    if (!mActive)
        mPhysics->reportCollision(mHitPosition, mHitNormal);
    mTaskScheduler->removeCollisionObject(mCollisionObject.get());
}

void Projectile::commitPositionChange()
{
    std::scoped_lock lock(mMutex);
    if (mTransformUpdatePending)
    {
        mCollisionObject->setWorldTransform(mLocalTransform);
        mTransformUpdatePending = false;
    }
}

void Projectile::setPosition(const osg::Vec3f &position)
{
    std::scoped_lock lock(mMutex);
    mLocalTransform.setOrigin(Misc::Convert::toBullet(position));
    mTransformUpdatePending = true;
}

osg::Vec3f Projectile::getPosition() const
{
    std::scoped_lock lock(mMutex);
    return Misc::Convert::toOsg(mLocalTransform.getOrigin());
}

bool Projectile::canTraverseWater() const
{
    return mCanCrossWaterSurface;
}

void Projectile::hit(const MWWorld::Ptr& target, btVector3 pos, btVector3 normal)
{
    if (!mActive.load(std::memory_order_acquire))
        return;
    std::scoped_lock lock(mMutex);
    mHitTarget = target;
    mHitPosition = pos;
    mHitNormal = normal;
    mActive.store(false, std::memory_order_release);
}

MWWorld::Ptr Projectile::getCaster() const
{
    std::scoped_lock lock(mMutex);
    return mCaster;
}

void Projectile::setCaster(const MWWorld::Ptr& caster)
{
    std::scoped_lock lock(mMutex);
    mCaster = caster;
}

void Projectile::setValidTargets(const std::vector<MWWorld::Ptr>& targets)
{
    std::scoped_lock lock(mMutex);
    mValidTargets = targets;
}

bool Projectile::isValidTarget(const MWWorld::Ptr& target) const
{
    std::scoped_lock lock(mMutex);
    if (mCaster == target)
        return false;

    if (target.isEmpty() || mValidTargets.empty())
        return true;

    bool validTarget = false;
    for (const auto& targetActor : mValidTargets)
    {
        if (targetActor == target)
        {
            validTarget = true;
            break;
        }
    }
    return validTarget;
}

std::optional<btVector3> Projectile::getWaterHitPosition()
{
    return std::exchange(mWaterHitPosition, std::nullopt);
}

void Projectile::setWaterHitPosition(btVector3 pos)
{
    if (mCrossedWaterSurface)
        return;
    mCrossedWaterSurface = true;
    mWaterHitPosition = pos;
}

}
