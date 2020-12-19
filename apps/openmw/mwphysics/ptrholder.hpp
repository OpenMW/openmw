#ifndef OPENMW_MWPHYSICS_PTRHOLDER_H
#define OPENMW_MWPHYSICS_PTRHOLDER_H

#include <mutex>

#include "../mwworld/ptr.hpp"

namespace MWPhysics
{
    class PtrHolder
    {
    public:
        virtual ~PtrHolder() {}

        void updatePtr(const MWWorld::Ptr& updated)
        {
            std::scoped_lock lock(mMutex);
            mPtr = updated;
        }

        MWWorld::Ptr getPtr()
        {
            std::scoped_lock lock(mMutex);
            return mPtr;
        }

        MWWorld::ConstPtr getPtr() const
        {
            std::scoped_lock lock(mMutex);
            return mPtr;
        }

    protected:
        MWWorld::Ptr mPtr;

    private:
        mutable std::mutex mMutex;
    };
}

#endif
