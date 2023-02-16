#ifndef CSM_WOLRD_COLLECTION_H
#define CSM_WOLRD_COLLECTION_H

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <QVariant>

#include <components/misc/strings/lower.hpp>

#include "collectionbase.hpp"
#include "columnbase.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "record.hpp"
#include "ref.hpp"

namespace CSMWorld
{
    /// \brief Access to ID field in records
    template <typename ESXRecordT>
    struct IdAccessor
    {
        void setId(ESXRecordT& record, const ESM::RefId& id) const;
        const ESM::RefId getId(const ESXRecordT& record) const;
    };

    template <typename ESXRecordT>
    void IdAccessor<ESXRecordT>::setId(ESXRecordT& record, const ESM::RefId& id) const
    {
        record.mId = id;
    }

    template <typename ESXRecordT>
    const ESM::RefId IdAccessor<ESXRecordT>::getId(const ESXRecordT& record) const
    {
        return record.mId;
    }

    template <>
    inline void IdAccessor<Land>::setId(Land& record, const ESM::RefId& id) const
    {
        int x = 0, y = 0;

        Land::parseUniqueRecordId(id.getRefIdString(), x, y);
        record.mX = x;
        record.mY = y;
    }

    template <>
    inline void IdAccessor<LandTexture>::setId(LandTexture& record, const ESM::RefId& id) const
    {
        int plugin = 0;
        int index = 0;

        LandTexture::parseUniqueRecordId(id.getRefIdString(), plugin, index);
        record.mPluginIndex = plugin;
        record.mIndex = index;
    }

    template <>
    inline const ESM::RefId IdAccessor<Land>::getId(const Land& record) const
    {
        return ESM::RefId::stringRefId(Land::createUniqueRecordId(record.mX, record.mY));
    }

    template <>
    inline const ESM::RefId IdAccessor<LandTexture>::getId(const LandTexture& record) const
    {
        return ESM::RefId::stringRefId(LandTexture::createUniqueRecordId(record.mPluginIndex, record.mIndex));
    }

    /// \brief Single-type record collection
    template <typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT>>
    class Collection : public CollectionBase
    {
    public:
        typedef ESXRecordT ESXRecord;

    private:
        std::vector<std::unique_ptr<Record<ESXRecordT>>> mRecords;
        std::map<ESM::RefId, int> mIndex;
        std::vector<Column<ESXRecordT>*> mColumns;

        // not implemented
        Collection(const Collection&);
        Collection& operator=(const Collection&);

    protected:
        const std::vector<std::unique_ptr<Record<ESXRecordT>>>& getRecords() const;

        bool reorderRowsImp(int baseIndex, const std::vector<int>& newOrder);
        ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
        /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
        ///
        /// \return Success?

        int cloneRecordImp(const std::string& origin, const std::string& dest, UniversalId::Type type);
        ///< Returns the index of the clone.

        int touchRecordImp(const std::string& id);
        ///< Returns the index of the record on success, -1 on failure.

    public:
        Collection();

        virtual ~Collection();

        void add(const ESXRecordT& record);
        ///< Add a new record (modified)

        int getSize() const override;

        ESM::RefId getId(int index) const override;

        int getIndex(const ESM::RefId& id) const override;

        int getColumns() const override;

        QVariant getData(int index, int column) const override;

        void setData(int index, int column, const QVariant& data) override;

        const ColumnBase& getColumn(int column) const override;

        virtual void merge();
        ///< Merge modified into base.

        virtual void purge();
        ///< Remove records that are flagged as erased.

        void removeRows(int index, int count) override;

        void appendBlankRecord(const ESM::RefId& id, UniversalId::Type type = UniversalId::Type_None) override;
        ///< \param type Will be ignored, unless the collection supports multiple record types

        void cloneRecord(
            const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type) override;

        bool touchRecord(const ESM::RefId& id) override;
        ///< Change the state of a record from base to modified, if it is not already.
        ///  \return True if the record was changed.

        int searchId(const ESM::RefId& id) const override;
        ////< Search record with \a id.
        /// \return index of record (if found) or -1 (not found)

        void replace(int index, std::unique_ptr<RecordBase> record) override;
        ///< If the record type does not match, an exception is thrown.
        ///
        /// \attention \a record must not change the ID.

        void appendRecord(std::unique_ptr<RecordBase> record, UniversalId::Type type = UniversalId::Type_None) override;
        ///< If the record type does not match, an exception is thrown.
        ///< \param type Will be ignored, unless the collection supports multiple record types

        const Record<ESXRecordT>& getRecord(const ESM::RefId& id) const override;

        const Record<ESXRecordT>& getRecord(int index) const override;

        int getAppendIndex(const ESM::RefId& id, UniversalId::Type type = UniversalId::Type_None) const override;
        ///< \param type Will be ignored, unless the collection supports multiple record types

        std::vector<ESM::RefId> getIds(bool listDeleted = true) const override;
        ///< Return a sorted collection of all IDs
        ///
        /// \param listDeleted include deleted record in the list

        virtual void insertRecord(
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type = UniversalId::Type_None);
        ///< Insert record before index.
        ///
        /// If the record type does not match, an exception is thrown.
        ///
        /// If the index is invalid either generally (by being out of range) or for the particular
        /// record, an exception is thrown.

        bool reorderRows(int baseIndex, const std::vector<int>& newOrder) override;
        ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
        /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
        ///
        /// \return Success?

        void addColumn(Column<ESXRecordT>* column);

        void setRecord(int index, std::unique_ptr<Record<ESXRecordT>> record);
        ///< \attention This function must not change the ID.

        NestableColumn* getNestableColumn(int column) const;
    };

    template <typename ESXRecordT, typename IdAccessorT>
    const std::vector<std::unique_ptr<Record<ESXRecordT>>>& Collection<ESXRecordT, IdAccessorT>::getRecords() const
    {
        return mRecords;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRowsImp(int baseIndex, const std::vector<int>& newOrder)
    {
        if (!newOrder.empty())
        {
            int size = static_cast<int>(newOrder.size());

            // check that all indices are present
            std::vector<int> test(newOrder);
            std::sort(test.begin(), test.end());
            if (*test.begin() != 0 || *--test.end() != size - 1)
                return false;

            // reorder records
            std::vector<std::unique_ptr<Record<ESXRecordT>>> buffer(size);

            for (int i = 0; i < size; ++i)
            {
                buffer[newOrder[i]] = std::move(mRecords[baseIndex + i]);
                if (buffer[newOrder[i]])
                    buffer[newOrder[i]]->setModified(buffer[newOrder[i]]->get());
            }

            std::move(buffer.begin(), buffer.end(), mRecords.begin() + baseIndex);

            // adjust index
            for (auto& [id, index] : mIndex)
                if (index >= baseIndex && index < baseIndex + size)
                    index = newOrder.at(index - baseIndex) + baseIndex;
        }

        return true;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::cloneRecordImp(
        const std::string& origin, const std::string& destination, UniversalId::Type type)
    {
        auto copy = std::make_unique<Record<ESXRecordT>>();
        copy->mModified = getRecord(ESM::RefId::stringRefId(origin)).get();
        copy->mState = RecordBase::State_ModifiedOnly;
        IdAccessorT().setId(copy->get(), ESM::RefId::stringRefId(destination));

        if (type == UniversalId::Type_Reference)
        {
            CSMWorld::CellRef* ptr = (CSMWorld::CellRef*)&copy->mModified;
            ptr->mRefNum.mIndex = 0;
        }
        ESM::RefId destinationRefId = ESM::RefId::stringRefId(destination);
        int index = getAppendIndex(destinationRefId, type);
        insertRecord(std::move(copy), getAppendIndex(destinationRefId, type));

        return index;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::touchRecordImp(const std::string& id)
    {
        int index = getIndex(ESM::RefId::stringRefId(id));
        Record<ESXRecordT>& record = *mRecords.at(index);
        if (record.isDeleted())
        {
            throw std::runtime_error("attempt to touch deleted record");
        }

        if (!record.isModified())
        {
            record.setModified(record.get());
            return index;
        }

        return -1;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::cloneRecord(
        const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type)
    {
        cloneRecordImp(origin.getRefIdString(), destination.getRefIdString(), type);
    }

    template <>
    inline void Collection<Land, IdAccessor<Land>>::cloneRecord(
        const ESM::RefId& origin, const ESM::RefId& destination, const UniversalId::Type type)
    {
        int index = cloneRecordImp(origin.getRefIdString(), destination.getRefIdString(), type);
        mRecords.at(index)->get().setPlugin(0);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::touchRecord(const ESM::RefId& id)
    {
        return touchRecordImp(id.getRefIdString()) != -1;
    }

    template <>
    inline bool Collection<Land, IdAccessor<Land>>::touchRecord(const ESM::RefId& id)
    {
        int index = touchRecordImp(id.getRefIdString());
        if (index >= 0)
        {
            mRecords.at(index)->get().setPlugin(0);
            return true;
        }

        return false;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::Collection()
    {
    }

    template <typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::~Collection()
    {
        for (typename std::vector<Column<ESXRecordT>*>::iterator iter(mColumns.begin()); iter != mColumns.end(); ++iter)
            delete *iter;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::add(const ESXRecordT& record)
    {
        const ESM::RefId id = IdAccessorT().getId(record);

        auto iter = mIndex.find(id);

        if (iter == mIndex.end())
        {
            auto record2 = std::make_unique<Record<ESXRecordT>>();
            record2->mState = Record<ESXRecordT>::State_ModifiedOnly;
            record2->mModified = record;

            insertRecord(std::move(record2), getAppendIndex(id));
        }
        else
        {
            mRecords[iter->second]->setModified(record);
        }
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getSize() const
    {
        return mRecords.size();
    }

    template <typename ESXRecordT, typename IdAccessorT>
    ESM::RefId Collection<ESXRecordT, IdAccessorT>::getId(int index) const
    {
        return IdAccessorT().getId(mRecords.at(index)->get());
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getIndex(const ESM::RefId& id) const
    {
        int index = searchId(id);

        if (index == -1)
            throw std::runtime_error("invalid ID: " + id.getRefIdString());

        return index;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getColumns() const
    {
        return mColumns.size();
    }

    template <typename ESXRecordT, typename IdAccessorT>
    QVariant Collection<ESXRecordT, IdAccessorT>::getData(int index, int column) const
    {
        return mColumns.at(column)->get(*mRecords.at(index));
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setData(int index, int column, const QVariant& data)
    {
        return mColumns.at(column)->set(*mRecords.at(index), data);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    const ColumnBase& Collection<ESXRecordT, IdAccessorT>::getColumn(int column) const
    {
        return *mColumns.at(column);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    NestableColumn* Collection<ESXRecordT, IdAccessorT>::getNestableColumn(int column) const
    {
        if (column < 0 || column >= static_cast<int>(mColumns.size()))
            throw std::runtime_error("column index out of range");

        return mColumns.at(column);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::addColumn(Column<ESXRecordT>* column)
    {
        mColumns.push_back(column);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::merge()
    {
        for (typename std::vector<std::unique_ptr<Record<ESXRecordT>>>::iterator iter(mRecords.begin());
             iter != mRecords.end(); ++iter)
            (*iter)->merge();

        purge();
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::purge()
    {
        int i = 0;

        while (i < static_cast<int>(mRecords.size()))
        {
            if (mRecords[i]->isErased())
                removeRows(i, 1);
            else
                ++i;
        }
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::removeRows(int index, int count)
    {
        mRecords.erase(mRecords.begin() + index, mRecords.begin() + index + count);

        auto iter = mIndex.begin();

        while (iter != mIndex.end())
        {
            if (iter->second >= index)
            {
                if (iter->second >= index + count)
                {
                    iter->second -= count;
                    ++iter;
                }
                else
                {
                    iter = mIndex.erase(iter);
                }
            }
            else
                ++iter;
        }
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::appendBlankRecord(const ESM::RefId& id, UniversalId::Type type)
    {
        ESXRecordT record;
        IdAccessorT().setId(record, id);
        record.blank();

        auto record2 = std::make_unique<Record<ESXRecordT>>();
        record2->mState = Record<ESXRecordT>::State_ModifiedOnly;
        record2->mModified = record;

        insertRecord(std::move(record2), getAppendIndex(id, type), type);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::searchId(const ESM::RefId& id) const
    {
        const auto iter = mIndex.find(id);

        if (iter == mIndex.end())
            return -1;

        return iter->second;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::replace(int index, std::unique_ptr<RecordBase> record)
    {
        std::unique_ptr<Record<ESXRecordT>> tmp(static_cast<Record<ESXRecordT>*>(record.release()));
        mRecords.at(index) = std::move(tmp);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::appendRecord(std::unique_ptr<RecordBase> record, UniversalId::Type type)
    {
        int index = getAppendIndex(IdAccessorT().getId(static_cast<Record<ESXRecordT>*>(record.get())->get()), type);
        insertRecord(std::move(record), index, type);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getAppendIndex(const ESM::RefId& id, UniversalId::Type type) const
    {
        return static_cast<int>(mRecords.size());
    }

    template <typename ESXRecordT, typename IdAccessorT>
    std::vector<ESM::RefId> Collection<ESXRecordT, IdAccessorT>::getIds(bool listDeleted) const
    {
        std::vector<ESM::RefId> ids;

        for (auto iter = mIndex.begin(); iter != mIndex.end(); ++iter)
        {
            if (listDeleted || !mRecords[iter->second]->isDeleted())
                ids.push_back(IdAccessorT().getId(mRecords[iter->second]->get()));
        }

        return ids;
    }

    template <typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord(const ESM::RefId& id) const
    {
        int index = getIndex(id);
        return *mRecords.at(index);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord(int index) const
    {
        return *mRecords.at(index);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::insertRecord(
        std::unique_ptr<RecordBase> record, int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<ESXRecordT>> record2(static_cast<Record<ESXRecordT>*>(record.release()));
        ESM::RefId id = IdAccessorT().getId(record2->get());

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin() + index, std::move(record2));

        if (index < size - 1)
        {
            for (auto& [key, value] : mIndex)
            {
                if (value >= index)
                    ++value;
            }
        }

        mIndex.insert(std::make_pair(id, index));
    }

    template <typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setRecord(int index, std::unique_ptr<Record<ESXRecordT>> record)
    {
        if (IdAccessorT().getId(mRecords.at(index)->get()) != IdAccessorT().getId(record->get()))
            throw std::runtime_error("attempt to change the ID of a record");

        mRecords.at(index) = std::move(record);
    }

    template <typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRows(int baseIndex, const std::vector<int>& newOrder)
    {
        return false;
    }
}

#endif
