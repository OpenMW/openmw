#ifndef GAME_MWCLASS_CONTAINERUTIL_H
#define GAME_MWCLASS_CONTAINERUTIL_H

#include <components/esm_store/cell_store.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

namespace MWClass
{
    template<typename T>
    void insertIntoContainerStore (const MWWorld::Ptr& ptr,
        ESMS::CellRefList<T, MWWorld::RefData>& containerStore)
    {
        if (!ptr.isEmpty())
        {
            // TODO check stacking

            ESMS::LiveCellRef<T, MWWorld::RefData> cellRef(ptr.getCellRef(), ptr.get<T>()->base);
            cellRef.mData = ptr.getRefData();

            containerStore.list.push_back (cellRef);

        }
    }
}

#endif
