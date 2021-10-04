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

    protected:
        MWWorld::Ptr mPtr;
        std::unique_ptr<btCollisionObject> mCollisionObject;
    };
}

#endif
