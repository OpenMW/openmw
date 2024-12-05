#ifndef OPENMW_MWPHYSICS_PROJECTILECONVEXCALLBACK_H
#define OPENMW_MWPHYSICS_PROJECTILECONVEXCALLBACK_H

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class btCollisionObject;

namespace MWPhysics
{
    class Projectile;

    class ProjectileConvexCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        explicit ProjectileConvexCallback(const btCollisionObject* caster, const btCollisionObject* me,
            const btVector3& from, const btVector3& to, Projectile& projectile)
            : btCollisionWorld::ClosestConvexResultCallback(from, to)
            , mCaster(caster)
            , mMe(me)
            , mProjectile(projectile)
        {
        }

        btScalar addSingleResult(btCollisionWorld::LocalConvexResult& result, bool normalInWorldSpace) override;

    private:
        const btCollisionObject* mCaster;
        const btCollisionObject* mMe;
        Projectile& mProjectile;
    };
}

#endif
