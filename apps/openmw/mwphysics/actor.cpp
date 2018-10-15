#include "actor.hpp"

#include <BulletCollision/CollisionShapes/btCylinderShape.h>
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
        // There are basically four shapes that are sensible to use for actor collisions.
        // - 1: Axis-Aligned Bounding Boxes, or AABBs. These are what vanilla Morrowind uses.
        // They make world geometry seem to collide differently at different angles, e.g. it's easier to hit the sides of 45 degree hallways.
        // - 2: Cylinders. These are like AABBs, flat on the bottom and the sides, but colliding with rotated objects at the same local angle is always basically the same, precision excepted.
        // Bullet seems to give bad normals for cylinder collisions sometimes. This is mostly avoided or worked around, but for cylinder-cylinder collisions
        // it can trigger a weird bug that makes it so you can shove actors by jumping against them over and over.
        // - 3: Rounded cylinders. Bullet can actually do these, but they perform very badly and don't render properly in debug collision wireframe rendering.
        // These don't have the bad normals problem that cylinders have, at least at a high enough rounding, but again, they perform very badly.
        // - 4: Capsules. These are touted as some kind of pancea, and they'd be good for total conversions, but they break vanilla Morrowind level design.

        // It's true that Vanilla uses AABBs, but there's nowhere that would be SIGNIFICANTLY different from vanilla when using cylinders instead. Capsules, on the other hand, break some staircases.
        // See https://gitlab.com/OpenMW/openmw/issues/4247 for more details.

        // Also, vanilla's movement solver is kind of screwed up in the first place, so a good movement solver is always going to be different anyway, even if it used AABBs.
        // An example of this is how some meshes you're supposed to step onto easily actually have tiny steep slopes at their starts. Vanilla just goes inside them then steps onto them.
        // OpenMW can't go into them. That would be bad.
        // AABBs and cylinders fail to slide onto them, getting stuck on the steep edge.
        // Capsules would let the player slide onto them, because of the tapered bottom, but capsules also break OTHER, more obvious/important vanilla level design examples, BECAUSE of the tapered bottom.

        // Cylinders are the best compromise for an actor collision hull shape.
        // It might be nice to allow content files to choose between cylinders and capsules in the future, probably as a part of post-1.0 dehardcoding.

        mShape.reset(new btCylinderShapeZ(btVector3(mHalfExtents.x(), mHalfExtents.x(), mHalfExtents.z())));
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
