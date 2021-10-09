#ifndef OPENMW_MWPHYSICS_PTRHOLDER_H
#define OPENMW_MWPHYSICS_PTRHOLDER_H

#include <mutex>
#include <memory>

#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include "../mwworld/ptr.hpp"

namespace MWPhysics
{
    class PtrHolder
    {
    public:
        virtual ~PtrHolder() = default;

        void updatePtr(const MWWorld::Ptr& updated)
        {
            mPtr = updated;
        }

        MWWorld::Ptr getPtr()
        {
            return mPtr;
        }

        btCollisionObject* getCollisionObject() const
        {
            return mCollisionObject.get();
        }

        void setVelocity(osg::Vec3f velocity)
        {
            mVelocity = velocity;
        }

        osg::Vec3f velocity()
        {
            return std::exchange(mVelocity, osg::Vec3f());
        }

        void setSimulationPosition(const osg::Vec3f& position)
        {
            mSimulationPosition = position;
        }

        osg::Vec3f getSimulationPosition() const
        {
            return mSimulationPosition;
        }

        void setPosition(const osg::Vec3f& position)
        {
            mPreviousPosition = mPosition;
            mPosition = position;
        }

        osg::Vec3f getPosition() const
        {
            return mPosition;
        }

        osg::Vec3f getPreviousPosition() const
        {
            return mPreviousPosition;
        }

    protected:
        MWWorld::Ptr mPtr;
        std::unique_ptr<btCollisionObject> mCollisionObject;
        osg::Vec3f mVelocity;
        osg::Vec3f mSimulationPosition;
        osg::Vec3f mPosition;
        osg::Vec3f mPreviousPosition;
    };
}

#endif
