#ifndef CSM_WOLRD_COLLECTION_H
#define CSM_WOLRD_COLLECTION_H

#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <functional>

#include <QVariant>

#include <components/misc/stringops.hpp>

#include "columnbase.hpp"

#include "collectionbase.hpp"

namespace CSMWorld
{
    /// \brief Access to ID field in records
    template<typename ESXRecordT>
    struct IdAccessor
    {
        std::string& getId (ESXRecordT& record);

        const std::string getId (const ESXRecordT& record) const;
    };

    template<typename ESXRecordT>
    std::string& IdAccessor<ESXRecordT>::getId (ESXRecordT& record)
    {
        return record.mId;
    }

    template<typename ESXRecordT>
    const std::string IdAccessor<ESXRecordT>::getId (const ESXRecordT& record) const
    {
        return record.mId;
    }

    /// \brief Single-type record collection
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
    class Collection : public CollectionBase
    {
            std::vector<Record<ESXRecordT> > mRecords;
            std::map<std::string, int> mIndex;
            std::vector<Column<ESXRecordT> *> mColumns;

            // not implemented
            Collection (const Collection&);
            Collection& operator= (const Collection&);

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

            virtual int getAppendIndex (UniversalId::Type type = UniversalId::Type_None) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs
            ///
            /// \param listDeleted include deleted record in the list

            void addColumn (Column<ESXRecordT> *column);

            void setRecord (int index, const Record<ESXRecordT>& record);
            ///< \attention This function must not change the ID.
    };

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

            mRecords.push_back (record2);
            mIndex.insert (std::make_pair (Misc::StringUtils::lowerCase (id), mRecords.size()-1));
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
        IdAccessorT().getId (record) = id;
        record.blank();
        add (record);
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
        mRecords.push_back (dynamic_cast<const Record<ESXRecordT>&> (record));
        mIndex.insert (std::make_pair (Misc::StringUtils::lowerCase (IdAccessorT().getId (
            dynamic_cast<const Record<ESXRecordT>&> (record).get())),
            mRecords.size()-1));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int Collection<ESXRecordT, IdAccessorT>::getAppendIndex (UniversalId::Type type) const
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
    void Collection<ESXRecordT, IdAccessorT>::setRecord (int index, const Record<ESXRecordT>& record)
    {
        if (IdAccessorT().getId (mRecords.at (index).get())!=IdAccessorT().getId (record.get()))
            throw std::runtime_error ("attempt to change the ID of a record");

        mRecords.at (index) = record;
    }
}

#endif
