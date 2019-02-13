#include "projectile.hpp"

#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include "../mwworld/class.hpp"

#include "collisiontype.hpp"

namespace MWPhysics
{
Projectile::Projectile(int projectileId, const osg::Vec3f& position, btCollisionWorld* world)
    : mCollisionWorld(world)
{
    mProjectileId = projectileId;

    mShape.reset(new btSphereShape(1.f));
    mConvexShape = static_cast<btConvexShape*>(mShape.get());

    mCollisionObject.reset(new btCollisionObject);
    mCollisionObject->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    mCollisionObject->setActivationState(DISABLE_DEACTIVATION);
    mCollisionObject->setCollisionShape(mShape.get());
    mCollisionObject->setUserPointer(this);

    setPosition(position);

    const int collisionMask = CollisionType_World | CollisionType_HeightMap |
        CollisionType_Actor | CollisionType_Door | CollisionType_Water;
    mCollisionWorld->addCollisionObject(mCollisionObject.get(), CollisionType_Projectile, collisionMask);
}

Projectile::~Projectile()
{
    if (mCollisionObject.get())
        mCollisionWorld->removeCollisionObject(mCollisionObject.get());
}

void Projectile::updateCollisionObjectPosition()
{
    btTransform tr = mCollisionObject->getWorldTransform();
   // osg::Vec3f scaledTranslation = mRotation * mMeshTranslation;
   // osg::Vec3f newPosition = scaledTranslation + mPosition;
    tr.setOrigin(Misc::Convert::toBullet(mPosition));
    mCollisionObject->setWorldTransform(tr);
}

void Projectile::setPosition(const osg::Vec3f &position)
{
    mPosition = position;
    updateCollisionObjectPosition();
}

osg::Vec3f Projectile::getPosition() const
{
    return mPosition;
}

}
