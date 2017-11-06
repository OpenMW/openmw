#ifndef OPENMW_MWPHYSICS_CLOSESTCOLLISION_H
#define OPENMW_MWPHYSICS_CLOSESTCOLLISION_H

#include <boost/optional.hpp>
#include <LinearMath/btVector3.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    struct Collision
    {
        const btCollisionObject* mObject;
        btScalar mFraction;
        btVector3 mNormal;
        btVector3 mPoint;
        btVector3 mEnd;
    };

    class ClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        ClosestNotMeConvexResultCallback(const btCollisionObject *me, const btVector3 &up, btScalar minSlopeDot);

        virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace);

    private:
        const btCollisionObject* mMe;
        const btVector3 mUp;
        const btScalar mMinSlopeDot;
    };

    boost::optional<Collision> getClosestCollision(const btCollisionObject& actorObject,
            const btVector3& source, const btVector3& destination, const btCollisionWorld& collisionWorld,
            int excludeFilterMask = 0);

    class CollisionWithObjectCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        CollisionWithObjectCallback(const btCollisionObject& object);

        virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace);

    private:
        const btCollisionObject* mObject;
    };

    boost::optional<Collision> getClosestCollisionWithStepUp(const btCollisionObject& actorObject,
            const btVector3& source, const btVector3& destination, const btCollisionWorld& collisionWorld);
}

#endif
