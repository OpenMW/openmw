#ifndef GAME_MWWORLD_LIVECELLREF_H
#define GAME_MWWORLD_LIVECELLREF_H

#include <components/esm/cellref.hpp>

#include "refdata.hpp"

namespace MWWorld
{
    class Ptr;
    class ESMStore;

    /// A reference to one object (of any type) in a cell.
    ///
    /// Constructing this with a CellRef instance in the constructor means that
    /// in practice (where D is RefData) the possibly mutable data is copied
    /// across to mData. If later adding data (such as position) to CellRef
    /// this would have to be manually copied across.
    template <typename X>
    struct LiveCellRef
    {
        LiveCellRef(const ESM::CellRef& cref, const X* b = NULL)
            : mBase(b), mRef(cref), mData(mRef)
        {}

        LiveCellRef(const X* b = NULL)
            : mBase(b), mData(mRef)
        {}

        // The object that this instance is based on.
        const X* mBase;

        /* Information about this instance, such as 3D location and
            rotation and individual type-dependent data.
        */
        ESM::CellRef mRef;

        /// runtime-data
        RefData mData;
    };

//    template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum);
}

#endif
