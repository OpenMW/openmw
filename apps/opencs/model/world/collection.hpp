#ifndef CSM_WOLRD_COLLECTION_H
#define CSM_WOLRD_COLLECTION_H

#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <functional>

#include <QVariant>

#include <components/misc/stringops.hpp>

#include "columnbase.hpp"
#include "collectionbase.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "ref.hpp"

namespace CSMWorld
{
    /// \brief Access to ID field in records
    template<typename ESXRecordT>
    struct IdAccessor
    {
        void setId(ESXRecordT& record, const std::string& id) const;
        const std::string getId (const ESXRecordT& record) const;
    };

    template<typename ESXRecordT>
    void IdAccessor<ESXRecordT>::setId(ESXRecordT& record, const std::string& id) const
    {
        record.mId = id;
    }

    template<typename ESXRecordT>
    const std::string IdAccessor<ESXRecordT>::getId (const ESXRecordT& record) const
    {
        return record.mId;
    }

    template<>
    inline void IdAccessor<Land>::setId (Land& record, const std::string& id) const
    {
        int x=0, y=0;

        Land::parseUniqueRecordId(id, x, y);
        record.mX = x;
        record.mY = y;
    }

    template<>
    inline void IdAccessor<LandTexture>::setId (LandTexture& record, const std::string& id) const
    {
        int plugin = 0;
        int index = 0;

        LandTexture::parseUniqueRecordId(id, plugin, index);
        record.mPluginIndex = plugin;
        record.mIndex = index;
    }

    template<>
    inline const std::string IdAccessor<Land>::getId (const Land& record) const
    {
        return Land::createUniqueRecordId(record.mX, record.mY);
    }

    template<>
    inline const std::string IdAccessor<LandTexture>::getId (const LandTexture& record) const
    {
        return LandTexture::createUniqueRecordId(record.mPluginIndex, record.mIndex);
    }

    /// \brief Single-type record collection
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class Collection : public CollectionBase
    {
        public:

            typedef ESXRecordT ESXRecord;

        private:

            std::vector<Record<ESXRecordT> > mRecords;
            std::map<std::string, int> mIndex;
            std::vector<Column<ESXRecordT> *> mColumns;

            // not implemented
            Collection (const Collection&);
            Collection& operator= (const Collection&);

        protected:

            const std::map<std::string, int>& getIdMap() const;

            const std::vector<Record<ESXRecordT> >& getRecords() const;

            bool reorderRowsImp (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            int cloneRecordImp (const std::string& origin, const std::string& dest,
                UniversalId::Type type);
            ///< Returns the index of the clone.

            int touchRecordImp (const std::string& id);
            ///< Returns the index of the record on success, -1 on failure.

        public:

            Collection();

            virtual ~Collection();

            void add (const ESXRecordT& record);
            ///< Add a new record (modified)

            virtual int getSize() const;

            virtual std::string getId (int index) const;

            virtual int getIndex (const std::string& id) const;

            virtual int getColumns() const;

            virtual QVariant getData (int index, int column) const;

            virtual void setData (int index, int column, const QVariant& data);

            virtual const ColumnBase& getColumn (int column) const;

            virtual void merge();
            ///< Merge modified into base.

            virtual void purge();
            ///< Remove records that are flagged as erased.

            virtual void removeRows (int index, int count) ;

            virtual void appendBlankRecord (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual void cloneRecord(const std::string& origin,
                                     const std::string& destination,
                                     const UniversalId::Type type);

            virtual bool touchRecord(const std::string& id);
            ///< Change the state of a record from base to modified, if it is not already.
            ///  \return True if the record was changed.

            virtual int searchId (const std::string& id) const;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record);
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            virtual void appendRecord (const RecordBase& record,
                UniversalId::Type type = UniversalId::Type_None);
            ///< If the record type does not match, an exception is thrown.
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual const Record<ESXRecordT>& getRecord (const std::string& id) const;

            virtual const Record<ESXRecordT>& getRecord (int index) const;

            virtual int getAppendIndex (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            virtual void insertRecord (const RecordBase& record, int index,
                UniversalId::Type type = UniversalId::Type_None);
            ///< Insert record before index.
            ///
            /// If the record type does not match, an exception is thrown.
            ///
            /// If the index is invalid either generally (by being out of range) or for the particular
            /// record, an exception is thrown.

            virtual bool reorderRows (int baseIndex, const std::vector<int>& newOrder);
            ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
            /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
            ///
            /// \return Success?

            void addColumn (Column<ESXRecordT> *column);

            void setRecord (int index, const Record<ESXRecordT>& record);
            ///< \attention This function must not change the ID.

            NestableColumn *getNestableColumn (int column) const;
    };

    template<typename ESXRecordT, typename IdAccessorT>
    const std::map<std::string, int>& Collection<ESXRecordT, IdAccessorT>::getIdMap() const
    {
        return mIndex;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const std::vector<Record<ESXRecordT> >& Collection<ESXRecordT, IdAccessorT>::getRecords() const
    {
        return mRecords;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRowsImp (int baseIndex,
        const std::vector<int>& newOrder)
    {
        if (!newOrder.empty())
        {
            int size = static_cast<int> (newOrder.size());

            // check that all indices are present
            std::vector<int> test (newOrder);
            std::sort (test.begin(), test.end());
            if (*test.begin()!=0 || *--test.end()!=size-1)
                return false;

            // reorder records
            std::vector<Record<ESXRecordT> > buffer (size);

            for (int i=0; i<size; ++i)
            {
                buffer[newOrder[i]] = mRecords [baseIndex+i];
                buffer[newOrder[i]].setModified (buffer[newOrder[i]].get());
            }

            std::copy (buffer.begin(), buffer.end(), mRecords.begin()+baseIndex);

            // adjust index
            for (std::map<std::string, int>::iterator iter (mIndex.begin()); iter!=mIndex.end();
                 ++iter)
                if (iter->second>=baseIndex && iter->second<baseIndex+size)
                    iter->second = newOrder.at (iter->second-baseIndex)+baseIndex;
        }

        return true;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::cloneRecordImp(const std::string& origin,
        const std::string& destination, UniversalId::Type type)
    {
        Record<ESXRecordT> copy;
        copy.mModified = getRecord(origin).get();
        copy.mState = RecordBase::State_ModifiedOnly;
        IdAccessorT().setId(copy.get(), destination);

        if (type == UniversalId::Type_Reference) {
            CSMWorld::CellRef* ptr = (CSMWorld::CellRef*) &copy.mModified;
            ptr->mRefNum.mIndex = 0;
        }

        int index = getAppendIndex(destination, type);
        insertRecord(copy, getAppendIndex(destination, type));

        return index;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::touchRecordImp(const std::string& id)
    {
        int index = getIndex(id);
        Record<ESXRecordT>& record = mRecords.at(index);
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

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::cloneRecord(const std::string& origin,
        const std::string& destination, const UniversalId::Type type)
    {
        cloneRecordImp(origin, destination, type);
    }

    template<>
    inline void Collection<Land, IdAccessor<Land> >::cloneRecord(const std::string& origin,
        const std::string& destination, const UniversalId::Type type)
    {
        int index = cloneRecordImp(origin, destination, type);
        mRecords.at(index).get().mPlugin = 0;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::touchRecord(const std::string& id)
    {
        return touchRecordImp(id) != -1;
    }

    template<>
    inline bool Collection<Land, IdAccessor<Land> >::touchRecord(const std::string& id)
    {
        int index = touchRecordImp(id);
        if (index >= 0)
        {
            mRecords.at(index).get().mPlugin = 0;
            return true;
        }

        return false;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::Collection()
    {}

    template<typename ESXRecordT, typename IdAccessorT>
    Collection<ESXRecordT, IdAccessorT>::~Collection()
    {
        for (typename std::vector<Column<ESXRecordT> *>::iterator iter (mColumns.begin()); iter!=mColumns.end(); ++iter)
            delete *iter;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::add (const ESXRecordT& record)
    {
        std::string id = Misc::StringUtils::lowerCase (IdAccessorT().getId (record));

        std::map<std::string, int>::iterator iter = mIndex.find (id);

        if (iter==mIndex.end())
        {
            Record<ESXRecordT> record2;
            record2.mState = Record<ESXRecordT>::State_ModifiedOnly;
            record2.mModified = record;

            insertRecord (record2, getAppendIndex (id));
        }
        else
        {
            mRecords[iter->second].setModified (record);
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getSize() const
    {
        return mRecords.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    std::string Collection<ESXRecordT, IdAccessorT>::getId (int index) const
    {
        return IdAccessorT().getId (mRecords.at (index).get());
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int  Collection<ESXRecordT, IdAccessorT>::getIndex (const std::string& id) const
    {
        int index = searchId (id);

        if (index==-1)
            throw std::runtime_error ("invalid ID: " + id);

        return index;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getColumns() const
    {
        return mColumns.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    QVariant Collection<ESXRecordT, IdAccessorT>::getData (int index, int column) const
    {
        return mColumns.at (column)->get (mRecords.at (index));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setData (int index, int column, const QVariant& data)
    {
        return mColumns.at (column)->set (mRecords.at (index), data);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const ColumnBase& Collection<ESXRecordT, IdAccessorT>::getColumn (int column) const
    {
        return *mColumns.at (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    NestableColumn *Collection<ESXRecordT, IdAccessorT>::getNestableColumn (int column) const
    {
        if (column < 0 || column >= static_cast<int>(mColumns.size()))
            throw std::runtime_error("column index out of range");

        return mColumns.at (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::addColumn (Column<ESXRecordT> *column)
    {
        mColumns.push_back (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::merge()
    {
        for (typename std::vector<Record<ESXRecordT> >::iterator iter (mRecords.begin()); iter!=mRecords.end(); ++iter)
            iter->merge();

        purge();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void  Collection<ESXRecordT, IdAccessorT>::purge()
    {
        int i = 0;

        while (i<static_cast<int> (mRecords.size()))
        {
            if (mRecords[i].isErased())
                removeRows (i, 1);
            else
                ++i;
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::removeRows (int index, int count)
    {
        mRecords.erase (mRecords.begin()+index, mRecords.begin()+index+count);

        typename std::map<std::string, int>::iterator iter = mIndex.begin();

        while (iter!=mIndex.end())
        {
            if (iter->second>=index)
            {
                if (iter->second>=index+count)
                {
                    iter->second -= count;
                    ++iter;
                }
                else
                {
                    mIndex.erase (iter++);
                }
            }
            else
                ++iter;
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void  Collection<ESXRecordT, IdAccessorT>::appendBlankRecord (const std::string& id,
        UniversalId::Type type)
    {
        ESXRecordT record;
        IdAccessorT().setId(record, id);
        record.blank();

        Record<ESXRecordT> record2;
        record2.mState = Record<ESXRecordT>::State_ModifiedOnly;
        record2.mModified = record;

        insertRecord (record2, getAppendIndex (id, type), type);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::searchId (const std::string& id) const
    {
        std::string id2 = Misc::StringUtils::lowerCase(id);

        std::map<std::string, int>::const_iterator iter = mIndex.find (id2);

        if (iter==mIndex.end())
            return -1;

        return iter->second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::replace (int index, const RecordBase& record)
    {
        mRecords.at (index) = dynamic_cast<const Record<ESXRecordT>&> (record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::appendRecord (const RecordBase& record,
        UniversalId::Type type)
    {
        insertRecord (record,
            getAppendIndex (IdAccessorT().getId (
            dynamic_cast<const Record<ESXRecordT>&> (record).get()), type), type);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getAppendIndex (const std::string& id,
        UniversalId::Type type) const
    {
        return static_cast<int> (mRecords.size());
    }

    template<typename ESXRecordT, typename IdAccessorT>
    std::vector<std::string> Collection<ESXRecordT, IdAccessorT>::getIds (bool listDeleted) const
    {
        std::vector<std::string> ids;

        for (typename std::map<std::string, int>::const_iterator iter = mIndex.begin();
            iter!=mIndex.end(); ++iter)
        {
            if (listDeleted || !mRecords[iter->second].isDeleted())
                ids.push_back (IdAccessorT().getId (mRecords[iter->second].get()));
        }

        return ids;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord (const std::string& id) const
    {
        int index = getIndex (id);
        return mRecords.at (index);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& Collection<ESXRecordT, IdAccessorT>::getRecord (int index) const
    {
        return mRecords.at (index);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::insertRecord (const RecordBase& record, int index,
        UniversalId::Type type)
    {
        if (index<0 || index>static_cast<int> (mRecords.size()))
            throw std::runtime_error ("index out of range");

        const Record<ESXRecordT>& record2 = dynamic_cast<const Record<ESXRecordT>&> (record);

        mRecords.insert (mRecords.begin()+index, record2);

        if (index<static_cast<int> (mRecords.size())-1)
        {
            for (std::map<std::string, int>::iterator iter (mIndex.begin()); iter!=mIndex.end();
                ++iter)
                 if (iter->second>=index)
                     ++(iter->second);
        }

        mIndex.insert (std::make_pair (Misc::StringUtils::lowerCase (IdAccessorT().getId (
            record2.get())), index));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void Collection<ESXRecordT, IdAccessorT>::setRecord (int index, const Record<ESXRecordT>& record)
    {
        if (Misc::StringUtils::lowerCase (IdAccessorT().getId (mRecords.at (index).get()))!=
            Misc::StringUtils::lowerCase (IdAccessorT().getId (record.get())))
            throw std::runtime_error ("attempt to change the ID of a record");

        mRecords.at (index) = record;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    bool Collection<ESXRecordT, IdAccessorT>::reorderRows (int baseIndex, const std::vector<int>& newOrder)
    {
        return false;
    }
}

#endif
