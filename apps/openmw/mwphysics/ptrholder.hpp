#ifndef OPENMW_MWPHYSICS_PTRHOLDER_H
#define OPENMW_MWPHYSICS_PTRHOLDER_H

#include "../mwworld/ptr.hpp"

namespace MWPhysics
{
    class PtrHolder
    {
    public:
        virtual ~PtrHolder() {}

        void updatePtr(const MWWorld::Ptr& updated)
        {
            mPtr = updated;
        }

        MWWorld::Ptr getPtr()
        {
            return mPtr;
        }

        MWWorld::ConstPtr getPtr() const
        {
            return mPtr;
        }

    protected:
        MWWorld::Ptr mPtr;
    };
}

#endif
