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

#include <components/esm/esmreader.hpp>

#include <components/misc/stringops.hpp>

#include "columnbase.hpp"
#include "universalid.hpp"

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

// Not in use. Temporarily removed so that the implementation of RefIdCollection can continue without
// these functions for now.
//            virtual void merge() = 0;
            ///< Merge modified into base.

//            virtual void purge() = 0;
            ///< Remove records that are flagged as erased.

            virtual void removeRows (int index, int count) = 0;

            virtual void appendBlankRecord (const std::string& id,
                UniversalId::Type type = UniversalId::Type_None) = 0;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual int searchId (const std::string& id) const = 0;
            ////< Search record with \a id.
            /// \return index of record (if found) or -1 (not found)

            virtual void replace (int index, const RecordBase& record) = 0;
            ///< If the record type does not match, an exception is thrown.
            ///
            /// \attention \a record must not change the ID.
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual void appendRecord (const RecordBase& record,
                UniversalId::Type type = UniversalId::Type_None) = 0;
            ///< If the record type does not match, an exception is thrown.

            virtual const RecordBase& getRecord (const std::string& id) const = 0;

            virtual const RecordBase& getRecord (int index) const = 0;

            virtual void load (ESM::ESMReader& reader, bool base,
                UniversalId::Type type = UniversalId::Type_None) = 0;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual int getAppendIndex (UniversalId::Type type = UniversalId::Type_None) const = 0;
            ///< \param type Will be ignored, unless the collection supports multiple record types
    };

    ///< \brief Access to ID field in records
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

    ///< \brief Collection of ID-based records
    template<typename ESXRecordT, typename IdAccessorT = IdAccessor<ESXRecordT> >
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

            virtual void load (ESM::ESMReader& reader, bool base,
                UniversalId::Type type = UniversalId::Type_None);
            ///< \param type Will be ignored, unless the collection supports multiple record types

            virtual int getAppendIndex (UniversalId::Type type = UniversalId::Type_None) const;
            ///< \param type Will be ignored, unless the collection supports multiple record types

            void addColumn (Column<ESXRecordT> *column);
    };

    template<typename ESXRecordT, typename IdAccessorT>
    IdCollection<ESXRecordT, IdAccessorT>::IdCollection()
    {}

    template<typename ESXRecordT, typename IdAccessorT>
    IdCollection<ESXRecordT, IdAccessorT>::~IdCollection()
    {
        for (typename std::vector<Column<ESXRecordT> *>::iterator iter (mColumns.begin()); iter!=mColumns.end(); ++iter)
            delete *iter;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::add (const ESXRecordT& record)
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
    int IdCollection<ESXRecordT, IdAccessorT>::getSize() const
    {
        return mRecords.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    std::string IdCollection<ESXRecordT, IdAccessorT>::getId (int index) const
    {
        return IdAccessorT().getId (mRecords.at (index).get());
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int  IdCollection<ESXRecordT, IdAccessorT>::getIndex (const std::string& id) const
    {
        int index = searchId (id);

        if (index==-1)
            throw std::runtime_error ("invalid ID: " + id);

        return index;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::getColumns() const
    {
        return mColumns.size();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    QVariant IdCollection<ESXRecordT, IdAccessorT>::getData (int index, int column) const
    {
        return mColumns.at (column)->get (mRecords.at (index));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::setData (int index, int column, const QVariant& data)
    {
        return mColumns.at (column)->set (mRecords.at (index), data);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const ColumnBase& IdCollection<ESXRecordT, IdAccessorT>::getColumn (int column) const
    {
        return *mColumns.at (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::addColumn (Column<ESXRecordT> *column)
    {
        mColumns.push_back (column);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::merge()
    {
        for (typename std::vector<Record<ESXRecordT> >::iterator iter (mRecords.begin()); iter!=mRecords.end(); ++iter)
            iter->merge();

        purge();
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void  IdCollection<ESXRecordT, IdAccessorT>::purge()
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
    void IdCollection<ESXRecordT, IdAccessorT>::removeRows (int index, int count)
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

    template<typename ESXRecordT, typename IdAccessorT>
    void  IdCollection<ESXRecordT, IdAccessorT>::appendBlankRecord (const std::string& id,
        UniversalId::Type type)
    {
        ESXRecordT record;
        IdAccessorT().getId (record) = id;
        record.blank();
        add (record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::searchId (const std::string& id) const
    {
        std::string id2 = Misc::StringUtils::lowerCase(id);

        std::map<std::string, int>::const_iterator iter = mIndex.find (id2);

        if (iter==mIndex.end())
            return -1;

        return iter->second;
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::replace (int index, const RecordBase& record)
    {
        mRecords.at (index) = dynamic_cast<const Record<ESXRecordT>&> (record);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::appendRecord (const RecordBase& record,
        UniversalId::Type type)
    {
        mRecords.push_back (dynamic_cast<const Record<ESXRecordT>&> (record));
        mIndex.insert (std::make_pair (Misc::StringUtils::lowerCase (IdAccessorT().getId (
            dynamic_cast<const Record<ESXRecordT>&> (record).get())),
            mRecords.size()-1));
    }

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::load (ESM::ESMReader& reader, bool base,
        UniversalId::Type type)
    {
        std::string id = reader.getHNOString ("NAME");

        if (reader.isNextSub ("DELE"))
        {
            int index = searchId (id);

            reader.skipRecord();

            if (index==-1)
            {
                // deleting a record that does not exist

                // ignore it for now

                /// \todo report the problem to the user
            }
            else if (base)
            {
                removeRows (index, 1);
            }
            else
            {
                mRecords[index].mState = RecordBase::State_Deleted;
            }
        }
        else
        {
            ESXRecordT record;
            IdAccessorT().getId (record) = id;
            record.load (reader);

            int index = searchId (IdAccessorT().getId (record));

            if (index==-1)
            {
                // new record
                Record<ESXRecordT> record2;
                record2.mState = base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
                (base ? record2.mBase : record2.mModified) = record;

                appendRecord (record2);
            }
            else
            {
                // old record
                Record<ESXRecordT>& record2 = mRecords[index];

                if (base)
                    record2.mBase = record;
                else
                    record2.setModified (record);
            }
        }
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::getAppendIndex (UniversalId::Type type) const
    {
        return static_cast<int> (mRecords.size());
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& IdCollection<ESXRecordT, IdAccessorT>::getRecord (const std::string& id) const
    {
        int index = getIndex (id);
        return mRecords.at (index);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    const Record<ESXRecordT>& IdCollection<ESXRecordT, IdAccessorT>::getRecord (int index) const
    {
        return mRecords.at (index);
    }

}

#endif
