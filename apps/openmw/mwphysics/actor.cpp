#include "actor.hpp"

#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btMinkowskiSumShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/resource/bulletshape.hpp>

#include "../mwworld/class.hpp"

#include "convert.hpp"
#include "collisiontype.hpp"

namespace MWPhysics
{


Actor::Actor(const MWWorld::Ptr& ptr, osg::ref_ptr<const Resource::BulletShape> shape, btCollisionWorld* world)
  : mCanWaterWalk(false), mWalkingOnWater(false)
  , mCollisionObject(nullptr), mForce(0.f, 0.f, 0.f), mOnGround(true), mOnSlope(false)
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
        // There are three shapes here, code given for convenience of testing/debugging.
        // The first is a cylinder with rounded edges. It's the best, but it seems to be a bit performance intensive.
        // The second is a cylinder. It would be idea, but Bullet gives bad/bogus normals for collisions of very close pairs of cylinders, causing weird collision glitches like jumping into NPCs making them move.
        // The third is a capsule. It causes problems with vanilla Morrowind's level design, because Morrowind was made and tested with cylinders. For example, some of the stairs in Ebonheart don't work. Good for full conversions.
        // Right here the capsule is enabled. This should be looked into before 1.0. If cylinders can be made to give good collision normals (for example, by compiling bullet with 64-bit floats), they should be used.
        // TODO: Should selection between these three shapes be a hidden option?
        if(false)
        {
            float fuzz = 8.0f;
            auto height = mHalfExtents.z()-fuzz;
            auto width = mHalfExtents.x()-fuzz;
            auto cylinder = new btCylinderShapeZ(btVector3(width, width, height));
            auto sphere = new btSphereShape(fuzz);
            mShape.reset(new btMinkowskiSumShape(cylinder, sphere));
        }
        else if(false)
            mShape.reset(new btCylinderShapeZ(btVector3(mHalfExtents.x(), mHalfExtents.x(), mHalfExtents.z())));
        else
            mShape.reset(new btCapsuleShapeZ(mHalfExtents.x(), 2*mHalfExtents.z() - 2*mHalfExtents.x()));
        mRotationallyInvariant = true;
    }
    else
    {
        mShape.reset(new btBoxShape(toBullet(mHalfExtents)));
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

void Actor::addCollisionMask(int collisionMask)
{
    mCollisionWorld->addCollisionObject(mCollisionObject.get(), CollisionType_Actor, collisionMask);
}

void Actor::updateCollisionMask()
{
    mCollisionWorld->removeCollisionObject(mCollisionObject.get());
    addCollisionMask(getCollisionMask());
}

int Actor::getCollisionMask()
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

    updateCollisionObjectPosition();
}

void Actor::updateCollisionObjectPosition()
{
    btTransform tr = mCollisionObject->getWorldTransform();
    osg::Vec3f scaledTranslation = mRotation * osg::componentMultiply(mMeshTranslation, mScale);
    osg::Vec3f newPosition = scaledTranslation + mPosition;
    tr.setOrigin(toBullet(newPosition));
    mCollisionObject->setWorldTransform(tr);
}

osg::Vec3f Actor::getCollisionObjectPosition() const
{
    return toOsg(mCollisionObject->getWorldTransform().getOrigin());
}

void Actor::setPosition(const osg::Vec3f &position)
{
    mPreviousPosition = mPosition;

    mPosition = position;
    updateCollisionObjectPosition();
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
    btTransform tr = mCollisionObject->getWorldTransform();
    mRotation = mPtr.getRefData().getBaseNode()->getAttitude();
    tr.setRotation(toBullet(mRotation));
    mCollisionObject->setWorldTransform(tr);

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
    mShape->setLocalScaling(toBullet(mScale));

    scaleVec = osg::Vec3f(scale,scale,scale);
    mPtr.getClass().adjustScale(mPtr, scaleVec, true);
    mRenderingScale = scaleVec;

    updateCollisionObjectPosition();
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
