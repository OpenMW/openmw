#ifndef CSM_WORLD_NESTEDADAPTORS_H
#define CSM_WORLD_NESTEDADAPTORS_H

#include <vector>

#include "universalid.hpp"
#include "nestedtablewrapper.hpp"
#include <components/esm/loadcont.hpp>
#include "record.hpp"
#include "refiddata.hpp"
#include "refidadapter.hpp"

namespace CSMWorld
{
    template <typename ESXRecordT>
    class InventoryHelper
    {
        CSMWorld::UniversalId::Type mType;

    public:

        InventoryHelper(CSMWorld::UniversalId::Type type) : mType(type) {};

        void setNestedTable(const RefIdColumn* column,
                            RefIdData& data,
                            int index,
                            const NestedTableWrapperBase& nestedTable)
        {
            getRecord(data, index).get().mInventory.mList = 
                (static_cast<const NestedTableWrapper<std::vector<ESM::ContItem> >&>(nestedTable)).mNestedTable;
        }
        
        NestedTableWrapperBase* nestedTable(const RefIdColumn* column,
                                            const RefIdData& data,
                                            int index) const
        {
            return new NestedTableWrapper<std::vector<ESM::ContItem> >(getRecord(data, index).get().mInventory.mList);
        }
        
    private:

        const Record<ESXRecordT>& getRecord(const RefIdData& data, int index) const
        {
            return dynamic_cast<const Record<ESXRecordT>&> (
                data.getRecord (RefIdData::LocalIndex (index, mType)));
        }

        Record<ESXRecordT>& getRecord(RefIdData& data, int index) const
        {
            return dynamic_cast<Record<ESXRecordT>&> (
                data.getRecord (RefIdData::LocalIndex (index, mType)));
        }
    };
}

#endif
