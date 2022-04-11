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
            std::clamp(position.x(), aabbMin.x(), aabbMax.x()),
            std::clamp(position.y(), aabbMin.y(), aabbMax.y()),
            std::clamp(position.z(), aabbMin.z(), aabbMax.z())
        );
        return nearest.distance(position) < radius;
    }

    template <class Ignore, class OnCollision>
    class HasSphereCollisionCallback final : public btBroadphaseAabbCallback
    {
    public:
        HasSphereCollisionCallback(const btVector3& position, const btScalar radius, const int mask, const int group,
                                   const Ignore& ignore, OnCollision* onCollision)
            : mPosition(position),
              mRadius(radius),
              mIgnore(ignore),
              mCollisionFilterMask(mask),
              mCollisionFilterGroup(group),
              mOnCollision(onCollision)
        {
        }

        bool process(const btBroadphaseProxy* proxy) override
        {
            if (mResult && mOnCollision == nullptr)
                return false;
            const auto collisionObject = static_cast<btCollisionObject*>(proxy->m_clientObject);
            if (mIgnore(collisionObject)
                || !needsCollision(*proxy)
                || !testAabbAgainstSphere(proxy->m_aabbMin, proxy->m_aabbMax, mPosition, mRadius))
                return true;
            mResult = true;
            if (mOnCollision != nullptr)
            {
                (*mOnCollision)(collisionObject);
                return true;
            }
            return !mResult;
        }

        bool getResult() const
        {
            return mResult;
        }

    private:
        btVector3 mPosition;
        btScalar mRadius;
        Ignore mIgnore;
        int mCollisionFilterMask;
        int mCollisionFilterGroup;
        OnCollision* mOnCollision;
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
