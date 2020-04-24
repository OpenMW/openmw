#ifndef OPENMW_MWPHYSICS_HASSPHERECOLLISIONCALLBACK_H
#define OPENMW_MWPHYSICS_HASSPHERECOLLISIONCALLBACK_H

#include <LinearMath/btVector3.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseInterface.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <algorithm>

namespace MWPhysics
{
    // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
    bool testAabbAgainstSphere(const btVector3& aabbMin, const btVector3& aabbMax,
        const btVector3& position, const btScalar radius)
    {
        const btVector3 nearest(
            std::max(aabbMin.x(), std::min(aabbMax.x(), position.x())),
            std::max(aabbMin.y(), std::min(aabbMax.y(), position.y())),
            std::max(aabbMin.z(), std::min(aabbMax.z(), position.z()))
        );
        return nearest.distance(position) < radius;
    }

    class HasSphereCollisionCallback final : public btBroadphaseAabbCallback
    {
    public:
        HasSphereCollisionCallback(const btVector3& position, const btScalar radius, btCollisionObject* object,
                const int mask, const int group)
            : mPosition(position),
              mRadius(radius),
              mCollisionObject(object),
              mCollisionFilterMask(mask),
              mCollisionFilterGroup(group)
        {
        }

        bool process(const btBroadphaseProxy* proxy) final
        {
            if (mResult)
                return false;
            const auto collisionObject = static_cast<btCollisionObject*>(proxy->m_clientObject);
            if (collisionObject == mCollisionObject)
                return true;
            if (needsCollision(*proxy))
                mResult = testAabbAgainstSphere(proxy->m_aabbMin, proxy->m_aabbMax, mPosition, mRadius);
            return !mResult;
        }

        bool getResult() const
        {
            return mResult;
        }

    private:
        btVector3 mPosition;
        btScalar mRadius;
        btCollisionObject* mCollisionObject;
        int mCollisionFilterMask;
        int mCollisionFilterGroup;
        bool mResult = false;

        bool needsCollision(const btBroadphaseProxy& proxy) const
        {
            bool collides = (proxy.m_collisionFilterGroup & mCollisionFilterMask) != 0;
            collides = collides && (mCollisionFilterGroup & proxy.m_collisionFilterMask);
            return collides;
        }
    };
}

#endif
