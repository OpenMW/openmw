#ifndef OPENMW_MWPHYSICS_PROJECTILE_H
#define OPENMW_MWPHYSICS_PROJECTILE_H

#include <atomic>
#include <memory>
#include <mutex>

#include "components/misc/convert.hpp"

#include "ptrholder.hpp"

class btCollisionObject;
class btCollisionShape;
class btConvexShape;
class btVector3;

namespace osg
{
    class Vec3f;
}

namespace Resource
{
    class BulletShape;
}

namespace MWPhysics
{
    class PhysicsTaskScheduler;
    class PhysicsSystem;

    class Projectile final : public PtrHolder
    {
    public:
        Projectile(const int projectileId, const MWWorld::Ptr& caster, const osg::Vec3f& position, PhysicsTaskScheduler* scheduler, PhysicsSystem* physicssystem);
        ~Projectile() override;

        btConvexShape* getConvexShape() const { return mConvexShape; }

        void commitPositionChange();

        void setPosition(const osg::Vec3f& position);

        btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
        }

        int getProjectileId() const
        {
            return mProjectileId;
        }

        bool isActive() const
        {
            return mActive.load(std::memory_order_acquire);
        }

        MWWorld::Ptr getTarget() const
        {
            assert(!mActive);
            return mHitTarget;
        }

        MWWorld::Ptr getCaster() const { return mCaster; }

        osg::Vec3f getHitPos() const
        {
            assert(!mActive);
            return Misc::Convert::toOsg(mHitPosition);
        }

        void hit(MWWorld::Ptr target, btVector3 pos, btVector3 normal);
        void activate();

    private:

        std::unique_ptr<btCollisionShape> mShape;
        btConvexShape* mConvexShape;

        std::unique_ptr<btCollisionObject> mCollisionObject;
        btTransform mLocalTransform;
        bool mTransformUpdatePending;
        std::atomic<bool> mActive;
        MWWorld::Ptr mCaster;
        MWWorld::Ptr mHitTarget;
        btVector3 mHitPosition;
        btVector3 mHitNormal;

        mutable std::mutex mPositionMutex;

        osg::Vec3f mPosition;

        PhysicsSystem *mPhysics;
        PhysicsTaskScheduler *mTaskScheduler;

        Projectile(const Projectile&);
        Projectile& operator=(const Projectile&);

        int mProjectileId;
    };

}


#endif
