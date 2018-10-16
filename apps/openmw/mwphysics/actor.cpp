#include "actor.hpp"

#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
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

    // the ratio between the side length and inradius of a regular octagon is 2tan(pi/8), which is equivalent to 2(sqrt(2)-1)
    static const double eir = 0.4142135623730951454746218587388284504413604736328125; // edge inradius ratio, divided by two.
    static bool inited_octagon = false;
    static btVector3 vertices[8];
    if(!inited_octagon)
    {
        vertices[0]  = btVector3( 1.0,  eir,  1.0);
        vertices[1]  = btVector3( eir,  1.0,  1.0);
        vertices[2]  = btVector3(-eir,  1.0,  1.0);
        vertices[3]  = btVector3(-1.0,  eir,  1.0);
        vertices[4]  = btVector3(-1.0, -eir,  1.0);
        vertices[5]  = btVector3(-eir, -1.0,  1.0);
        vertices[6]  = btVector3( eir, -1.0,  1.0);
        vertices[7]  = btVector3( 1.0, -eir,  1.0);
        inited_octagon = false;
    }
    
    auto tempshape = new btConvexHullShape();
    
    // we need to fill in the points in a special order to make the debug rendering of the mesh look good
    // fill in the side edges and top rim
    for(int i = 0; i < 8; i++)
    {
        auto vertex = vertices[i] * toBullet(mHalfExtents);
        tempshape->addPoint(vertex);
        tempshape->addPoint(vertex * btVector3(1,1,-1));
        tempshape->addPoint(vertex); // don't make a triangle shape; also causes the top rim to form
    }
    // fill in first top point again to prevent crossover and finish the rim
    tempshape->addPoint(vertices[0] * toBullet(mHalfExtents));
    for(int i = 0; i < 8; i++)
    {
        auto vertex = vertices[i] * toBullet(mHalfExtents);
        tempshape->addPoint(vertex * btVector3(1,1,-1));
    }
    // fill in first bottom point again to prevent crossover and finish the rim
    tempshape->addPoint(vertices[0] * toBullet(mHalfExtents) * btVector3(1,1,-1));

    mShape.reset(tempshape);
    mRotationallyInvariant = true;

    mConvexShape = static_cast<btConvexShape*>(mShape.get());

    mCollisionObject.reset(new btCollisionObject);
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(static_cast<PtrHolder*>(this));

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
