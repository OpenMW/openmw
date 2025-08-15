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

    public:
        ManualRef(const MWWorld::ESMStore& store, const ESM::RefId& name, const int count = 1);
        ManualRef(const MWWorld::ESMStore& store, const MWWorld::Ptr& template_, const int count = 1);

        ManualRef(const ManualRef&) = delete;
        ManualRef& operator=(const ManualRef&) = delete;

        ManualRef(ManualRef&&) noexcept = default;
        ManualRef& operator=(ManualRef&&) noexcept = default;

        const Ptr& getPtr() const { return mPtr; }
    };
}

#endif
