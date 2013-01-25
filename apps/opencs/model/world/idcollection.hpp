#ifndef CSM_WOLRD_IDCOLLECTION_H
#define CSM_WOLRD_IDCOLLECTION_H

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <functional>

#include <QVariant>

#include "columnbase.hpp"
#include <components/misc/stringops.hpp>

namespace CSMWorld
{
    class IdCollectionBase
    {
            // not implemented
            IdCollectionBase (const IdCollectionBase&);
            IdCollectionBase& operator= (const IdCollectionBase&);

        public:

            IdCollectionBase();

            virtual ~IdCollectionBase();

            virtual int getSize() const = 0;

            virtual std::string getId (int index) const = 0;

            virtual int getIndex (const std::string& id) const = 0;

            virtual int getColumns() const = 0;

            virtual const ColumnBase& getColumn (int column) const = 0;

            virtual QVariant getData (int index, int column) const = 0;

            virtual void setData (int index, int column, const QVariant& data) = 0;

            virtual void merge() = 0;
            ///< Merge modified into base.

            virtual void purge() = 0;
            ///< Remove records that are flagged as erased.

            virtual void removeRows (int index, int count) = 0;

            virtual void appendBlankRecord (const std::string& id) = 0;

            virtual int searchId (const std::string& id) const = 0;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record) = 0;
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            virtual void appendRecord (const RecordBase& record) = 0;
            ///< If the record type does not match, an exception is thrown.

            virtual std::string getId (const RecordBase& record) const = 0;
            ///< Return ID for \a record.
            ///
            /// \attention Throw san exception, if the type of \a record does not match.

            virtual const RecordBase& getRecord (const std::string& id) const = 0;
    };

    ///< \brief Collection of ID-based records
    template<typename ESXRecordT>
    class IdCollection : public IdCollectionBase
    {
            std::vector<Record<ESXRecordT> > mRecords;
            std::map<std::string, int> mIndex;
            std::vector<Column<ESXRecordT> *> mColumns;

            // not implemented
            IdCollection (const IdCollection&);
            IdCollection& operator= (const IdCollection&);

        public:

            IdCollection();

            virtual ~IdCollection();

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

            virtual void appendBlankRecord (const std::string& id);

            virtual int searchId (const std::string& id) const;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record);
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.

            virtual void appendRecord (const RecordBase& record);
            ///< If the record type does not match, an exception is thrown.

            virtual std::string getId (const RecordBase& record) const;
            ///< Return ID for \a record.
            ///
            /// \attention Throw san exception, if the type of \a record does not match.

            virtual const RecordBase& getRecord (const std::string& id) const;

            void addColumn (Column<ESXRecordT> *column);
    };

    template<typename ESXRecordT>
    IdCollection<ESXRecordT>::IdCollection()
    {}

    template<typename ESXRecordT>
    IdCollection<ESXRecordT>::~IdCollection()
    {
        for (typename std::vector<Column<ESXRecordT> *>::iterator iter (mColumns.begin()); iter!=mColumns.end(); ++iter)
            delete *iter;
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::add (const ESXRecordT& record)
    {
        std::string id = Misc::StringUtils::lowerCase(record.mId);

        std::map<std::string, int>::iterator iter = mIndex.find (id);

        if (iter==mIndex.end())
        {
            Record<ESXRecordT> record2;
            record2.mState = Record<ESXRecordT>::State_ModifiedOnly;
            record2.mModified = record;

            mRecords.push_back (record2);
            mIndex.insert (std::make_pair (id, mRecords.size()-1));
        }
        else
        {
            mRecords[iter->second].setModified (record);
        }
    }

    template<typename ESXRecordT>
    int IdCollection<ESXRecordT>::getSize() const
    {
        return mRecords.size();
    }

    template<typename ESXRecordT>
    std::string IdCollection<ESXRecordT>::getId (int index) const
    {
        return mRecords.at (index).get().mId;
    }

    template<typename ESXRecordT>
    int  IdCollection<ESXRecordT>::getIndex (const std::string& id) const
    {
        int index = searchId (id);

        if (index==-1)
            throw std::runtime_error ("invalid ID: " + id);

        return index;
    }

    template<typename ESXRecordT>
    int IdCollection<ESXRecordT>::getColumns() const
    {
        return mColumns.size();
    }

    template<typename ESXRecordT>
    QVariant IdCollection<ESXRecordT>::getData (int index, int column) const
    {
        return mColumns.at (column)->get (mRecords.at (index));
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::setData (int index, int column, const QVariant& data)
    {
        return mColumns.at (column)->set (mRecords.at (index), data);
    }

    template<typename ESXRecordT>
    const ColumnBase& IdCollection<ESXRecordT>::getColumn (int column) const
    {
        return *mColumns.at (column);
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::addColumn (Column<ESXRecordT> *column)
    {
        mColumns.push_back (column);
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::merge()
    {
        for (typename std::vector<Record<ESXRecordT> >::iterator iter (mRecords.begin()); iter!=mRecords.end(); ++iter)
            iter->merge();

        purge();
    }

    template<typename ESXRecordT>
    void  IdCollection<ESXRecordT>::purge()
    {
        mRecords.erase (std::remove_if (mRecords.begin(), mRecords.end(),
            std::mem_fun_ref (&Record<ESXRecordT>::isErased) // I want lambda :(
            ), mRecords.end());
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::removeRows (int index, int count)
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
                }
                else
                {
                    mIndex.erase (iter++);
                }
            }

            ++iter;
        }
    }

    template<typename ESXRecordT>
    void  IdCollection<ESXRecordT>::appendBlankRecord (const std::string& id)
    {
        ESXRecordT record;
        record.mId = id;
        record.blank();
        add (record);
    }

    template<typename ESXRecordT>
    int IdCollection<ESXRecordT>::searchId (const std::string& id) const
    {
        std::string id2 = Misc::StringUtils::lowerCase(id);

        std::map<std::string, int>::const_iterator iter = mIndex.find (id2);

        if (iter==mIndex.end())
            return -1;

        return iter->second;
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::replace (int index, const RecordBase& record)
    {
        mRecords.at (index) = dynamic_cast<const Record<ESXRecordT>&> (record);
    }

    template<typename ESXRecordT>
    void IdCollection<ESXRecordT>::appendRecord (const RecordBase& record)
    {
        mRecords.push_back (dynamic_cast<const Record<ESXRecordT>&> (record));
        mIndex.insert (std::make_pair (getId (record), mRecords.size()-1));
    }

    template<typename ESXRecordT>
    std::string IdCollection<ESXRecordT>::getId (const RecordBase& record) const
    {
        const Record<ESXRecordT>& record2 = dynamic_cast<const Record<ESXRecordT>&> (record);
        return (record2.isModified() ? record2.mModified : record2.mBase).mId;
    }

    template<typename ESXRecordT>
    const RecordBase& IdCollection<ESXRecordT>::getRecord (const std::string& id) const
    {
        int index = getIndex (id);
        return mRecords.at (index);
    }
}

#endif
