#ifndef GAME_MWWORLD_MANUALREF_H
#define GAME_MWWORLD_MANUALREF_H

#include <any>

#include "ptr.hpp"

namespace MWWorld
{
    /// \brief Manually constructed live cell ref. The resulting Ptr shares its lifetime with this ManualRef and must
    /// not be used past its end.
    class ManualRef
    {
        // Stores the ref (LiveCellRef<T>) by value.
        std::any mRef;
        Ptr mPtr;

        ManualRef(const ManualRef&);
        ManualRef& operator=(const ManualRef&);

    public:
        ManualRef(const MWWorld::ESMStore& store, const ESM::RefId& name, const int count = 1);
        ManualRef(const MWWorld::ESMStore& store, const MWWorld::Ptr& template_, const int count = 1);

        const Ptr& getPtr() const { return mPtr; }
    };
}

#endif
