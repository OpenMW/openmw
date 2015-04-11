#ifndef CSM_WORLD_NESTEDADAPTERS_H
#define CSM_WORLD_NESTEDADAPTERS_H

#include <vector>
#include <string>
#include <stdexcept>

#include "universalid.hpp"
#include "nestedtablewrapper.hpp"
#include "record.hpp"
#include "refiddata.hpp"
#include "refidadapter.hpp"
#include <components/esm/loadcont.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/effectlist.hpp>
#include <components/esm/loadmgef.hpp>

#include <QVariant>

/*! \brief
 * Nested adapter redirects responsibility to the helper class. Helper classes are polymorhpic (vide HelperBase and CastableHelper) and most likely templates.
 */

namespace CSMWorld
{
    class RefIdColumn;

    class HelperBase
    {
    protected:
        const CSMWorld::UniversalId::Type mType;

    public:
        HelperBase(CSMWorld::UniversalId::Type type);

        virtual ~HelperBase();

        virtual void setNestedTable(RefIdData& data,
                                    int index,
                                    const NestedTableWrapperBase& nestedTable) = 0;

        virtual NestedTableWrapperBase* nestedTable(const RefIdData& data,
                                                    int index) const = 0;

        virtual QVariant getNestedData(const CSMWorld::RefIdData& data,
                                       int index,
                                       int subRowIndex,
                                       int subColIndex) const = 0;

        virtual void removeNestedRow (RefIdData& data,
                                      int index,
                                      int rowToRemove) const = 0;

        virtual void setNestedData (RefIdData& data,
                                    int index,
                                    const QVariant& value,
                                    int subRowIndex,
                                    int subColIndex) const = 0;

        virtual void addNestedRow (RefIdData& data,
                                   int index,
                                   int position) const = 0;

        virtual int getNestedColumnsCount(const RefIdData& data) const = 0;

        virtual int getNestedRowsCount(const RefIdData& data,
                                       int index) const = 0;
    };

    template <typename ESXRecordT>
    class CastableHelper : public HelperBase
    {

    public:
        CastableHelper(CSMWorld::UniversalId::Type type)
            : HelperBase(type) {}

    protected:
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

    template <typename ESXRecordT>
    class SpellsHelper : public CastableHelper<ESXRecordT>
    {
    public:

        SpellsHelper(CSMWorld::UniversalId::Type type)
            : CastableHelper<ESXRecordT>(type) {}

        virtual void setNestedTable(RefIdData& data,
                                    int index,
                                    const NestedTableWrapperBase& nestedTable)
        {
            CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList =
                (static_cast<const NestedTableWrapper<std::vector<std::string> >&>(nestedTable)).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const RefIdData& data,
                                                    int index) const
        {
            return new NestedTableWrapper<std::vector<std::string> >(CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList);
        }

        virtual QVariant getNestedData(const CSMWorld::RefIdData& data,
                                       int index,
                                       int subRowIndex,
                                       int subColIndex) const
        {
            const std::string& content = CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList.at(subRowIndex);

            if (subColIndex == 0)
            {
                return QString::fromUtf8(content.c_str());
            }

            throw std::logic_error("Trying to access non-existing column in the nested table!");
        }

        virtual void removeNestedRow (RefIdData& data, int index, int rowToRemove) const
        {
            std::vector<std::string>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList;

            list.erase (list.begin () + rowToRemove);
        }

        void setNestedData (RefIdData& data,
                            int index,
                            const QVariant& value,
                            int subRowIndex,
                            int subColIndex) const
        {
            if (subColIndex == 0)
            {
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList.at(subRowIndex) = std::string(value.toString().toUtf8());
            }
            else
                throw std::logic_error("Trying to access non-existing column in the nested table!");
        }

        virtual void addNestedRow (RefIdData& data, int index, int position) const
        {
            std::vector<std::string>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList;

            std::string newString;
            if (position >= (int)list.size())
            {
                list.push_back(newString);
                return;
            }

            list.insert(list.begin()+position, newString);
        }

        virtual int getNestedColumnsCount(const RefIdData& data) const
        {
            return 1;
        }


        virtual int getNestedRowsCount(const RefIdData& data,
                                       int index) const
        {
            return CastableHelper<ESXRecordT>::getRecord(data, index).get().mSpells.mList.size();
        }

    };

    template <typename ESXRecordT>
    class DestinationsHelper : public CastableHelper<ESXRecordT>
    {
    public:

        DestinationsHelper(CSMWorld::UniversalId::Type type)
            : CastableHelper<ESXRecordT>(type) {}

        virtual void setNestedTable(RefIdData& data,
                                    int index,
                                    const NestedTableWrapperBase& nestedTable)
        {
            CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList =
                (static_cast<const NestedTableWrapper<std::vector<ESM::Transport::Dest> >&>(nestedTable)).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const RefIdData& data,
                                                    int index) const
        {
            return new NestedTableWrapper<std::vector<ESM::Transport::Dest> >(CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList);
        }

        virtual QVariant getNestedData(const CSMWorld::RefIdData& data,
                                       int index,
                                       int subRowIndex,
                                       int subColIndex) const
        {
            const ESM::Transport::Dest& content = CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex);

            switch (subColIndex)
            {
            case 0:
                return QString::fromUtf8(content.mCellName.c_str());

            case 1:
                return content.mPos.pos[0];

            case 2:
                return content.mPos.pos[1];

            case 3:
                return content.mPos.pos[2];

            case 4:
                return content.mPos.rot[0];

            case 5:
                return content.mPos.rot[1];

            case 6:
                return content.mPos.rot[2];

            default:
                throw std::logic_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void removeNestedRow (RefIdData& data, int index, int rowToRemove) const
        {
            std::vector<ESM::Transport::Dest>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList;

            list.erase (list.begin () + rowToRemove);
        }

        void setNestedData (RefIdData& data,
                            int index,
                            const QVariant& value,
                            int subRowIndex,
                            int subColIndex) const
        {
            switch(subColIndex)
            {
            case 0:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mCellName = std::string(value.toString().toUtf8().constData());
                break;

            case 1:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.pos[0] = value.toFloat();
                break;

            case 2:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.pos[1] = value.toFloat();
                break;

            case 3:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.pos[2] = value.toFloat();
                break;

            case 4:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.rot[0] = value.toFloat();
                break;

            case 5:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.rot[1] = value.toFloat();
                break;

            case 6:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.at(subRowIndex).mPos.rot[2] = value.toFloat();
                break;

            default:
                throw std::logic_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void addNestedRow (RefIdData& data, int index, int position) const
        {
            std::vector<ESM::Transport::Dest>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList;

            ESM::Position newPos;
            for (unsigned i = 0; i < 3; ++i)
            {
                newPos.pos[i] = 0;
                newPos.rot[i] = 0;
            }

            ESM::Transport::Dest newRow;
            newRow.mPos = newPos;
            newRow.mCellName = "";

            if (position >= (int)list.size())
            {
                list.push_back(newRow);
                return;
            }

            list.insert(list.begin()+position, newRow);
        }

        virtual int getNestedColumnsCount(const RefIdData& data) const
        {
            return 7;
        }

        virtual int getNestedRowsCount(const RefIdData& data,
                                       int index) const
        {
            return CastableHelper<ESXRecordT>::getRecord(data, index).get().mTransport.mList.size();
        }

    };

    template <typename ESXRecordT>
    class InventoryHelper : public CastableHelper<ESXRecordT>
    {
    public:

        InventoryHelper(CSMWorld::UniversalId::Type type)
            : CastableHelper<ESXRecordT>(type) {}

        virtual void setNestedTable(RefIdData& data,
                                    int index,
                                    const NestedTableWrapperBase& nestedTable)
        {
            CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList =
                (static_cast<const NestedTableWrapper<std::vector<ESM::ContItem> >&>(nestedTable)).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const RefIdData& data,
                                                    int index) const
        {
            return new NestedTableWrapper<std::vector<ESM::ContItem> >(CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList);
        }

        virtual QVariant getNestedData(const CSMWorld::RefIdData& data,
                                       int index,
                                       int subRowIndex,
                                       int subColIndex) const
        {
            const ESM::ContItem& content = CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList.at(subRowIndex);

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

        virtual void removeNestedRow (RefIdData& data, int index, int rowToRemove) const
        {
            std::vector<ESM::ContItem>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList;

            list.erase (list.begin () + rowToRemove);
        }

        void setNestedData (RefIdData& data,
                            int index,
                            const QVariant& value,
                            int subRowIndex,
                            int subColIndex) const
        {
            switch(subColIndex)
            {
            case 0:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList.at(subRowIndex).mItem.assign(std::string(value.toString().toUtf8().constData()));
                break;

            case 1:
                CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList.at(subRowIndex).mCount = value.toInt();
                break;

            default:
                throw std::logic_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void addNestedRow (RefIdData& data, int index, int position) const
        {
            std::vector<ESM::ContItem>& list = CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList;

            ESM::ContItem newRow = {0, {""}};
            if (position >= (int)list.size())
            {
                list.push_back(newRow);
                return;
            }

            list.insert(list.begin()+position, newRow);
        }

        virtual int getNestedColumnsCount(const RefIdData& data) const
        {
            return 2;
        }

        virtual int getNestedRowsCount(const RefIdData& data,
                                       int index) const
        {
            return CastableHelper<ESXRecordT>::getRecord(data, index).get().mInventory.mList.size();
        }

    };
}

#endif
