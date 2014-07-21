#ifndef CSM_WORLD_NESTEDADAPTORS_H
#define CSM_WORLD_NESTEDADAPTORS_H

#include <vector>
#include <stdexcept>

#include "universalid.hpp"
#include "nestedtablewrapper.hpp"
#include <components/esm/loadcont.hpp>
#include "record.hpp"
#include "refiddata.hpp"
#include "refidadapter.hpp"

#include <QVariant>

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
        
        QVariant getNestedData(const CSMWorld::RefIdColumn* column,
                               const CSMWorld::RefIdData& data,
                               int index,
                               int subRowIndex,
                               int subColIndex) const
        {
            const ESM::ContItem& content = getRecord(data, index).get().mInventory.mList.at(subRowIndex);

            switch (subColIndex)
            {
            case 0:
                return QString::fromUtf8(content.mItem.toString().c_str());
                
            case 1:
                return content.mCount;
                
            default:
                throw std::logic_error("Trying to access non-existing column in the nested table!");
            }
        }

        void removeNestedRow (const RefIdColumn *column, RefIdData& data, int index, int rowToRemove) const
        {
            std::vector<ESM::ContItem>& list = getRecord(data, index).get().mInventory.mList;

            list.erase (list.begin () + rowToRemove);
        }

        void setNestedData (const RefIdColumn *column,
                            RefIdData& data,
                            int index,
                            const QVariant& value,
                            int subRowIndex,
                            int subColIndex) const
        {
            switch(subColIndex)
            {
            case 0:
                getRecord(data, index).get().mInventory.mList.at(subRowIndex).mItem.assign(std::string(value.toString().toUtf8().constData()));
                break;

            case 1:
                getRecord(data, index).get().mInventory.mList.at(subRowIndex).mCount = value.toInt();
                break;
                
            default:
                throw std::logic_error("Trying to access non-existing column in the nested table!");
            }
        }
        
        void addNestedRow (const RefIdColumn *column, RefIdData& data, int index, int position) const
        {
            std::vector<ESM::ContItem>& list = getRecord(data, index).get().mInventory.mList;

            ESM::ContItem newRow = {0, ""};
            if (position >= (int)list.size())
            {
                list.push_back(newRow);
                return;
            }
            
            list.insert(list.begin()+position, newRow);
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
