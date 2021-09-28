#ifndef OPENMW_MWPHYSICS_PROJECTILE_H
#define OPENMW_MWPHYSICS_PROJECTILE_H

#include <atomic>
#include <memory>
#include <mutex>

#include <LinearMath/btVector3.h>

#include "ptrholder.hpp"

class btCollisionObject;
class btCollisionShape;
class btConvexShape;

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
        Projectile(const MWWorld::Ptr& caster, const osg::Vec3f& position, float radius, PhysicsTaskScheduler* scheduler, PhysicsSystem* physicssystem);
        ~Projectile() override;

        btConvexShape* getConvexShape() const { return mConvexShape; }

        void commitPositionChange();

        void setPosition(const osg::Vec3f& position);
        osg::Vec3f getPosition() const;

        btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
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

        MWWorld::Ptr getCaster() const;
        void setCaster(MWWorld::Ptr caster);

        void setHitWater()
        {
            mHitWater = true;
        }

        bool getHitWater() const
        {
            return mHitWater;
        }

        void hit(MWWorld::Ptr target, btVector3 pos, btVector3 normal);

        void setValidTargets(const std::vector<MWWorld::Ptr>& targets);
        bool isValidTarget(const MWWorld::Ptr& target) const;

        btVector3 getHitPosition() const
        {
            return mHitPosition;
        }

    private:

        std::unique_ptr<btCollisionShape> mShape;
        btConvexShape* mConvexShape;

        std::unique_ptr<btCollisionObject> mCollisionObject;
        bool mTransformUpdatePending;
        bool mHitWater;
        std::atomic<bool> mActive;
        MWWorld::Ptr mCaster;
        MWWorld::Ptr mHitTarget;
        osg::Vec3f mPosition;
        btVector3 mHitPosition;
        btVector3 mHitNormal;

        std::vector<MWWorld::Ptr> mValidTargets;

        mutable std::mutex mMutex;

        PhysicsSystem *mPhysics;
        PhysicsTaskScheduler *mTaskScheduler;

        Projectile(const Projectile&);
        Projectile& operator=(const Projectile&);
    };

}


#endif
