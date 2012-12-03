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

#include "record.hpp"

namespace CSMWorld
{
    template<typename ESXRecordT>
    struct Column
    {
        std::string mTitle;

        Column (const std::string& title) : mTitle (title) {}

        virtual ~Column() {}

        virtual QVariant get (const Record<ESXRecordT>& record) const = 0;

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            throw std::logic_error ("Column " + mTitle + " is not editable");
        }

        virtual bool isEditable() const = 0;
    };

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

            virtual int getColumns() const = 0;

            virtual std::string getTitle (int column) const = 0;

            virtual QVariant getData (int index, int column) const = 0;

            virtual void setData (int index, int column, const QVariant& data) = 0;

            virtual bool isEditable (int column) const = 0;

            virtual void merge() = 0;
            ///< Merge modified into base.

            virtual void purge() = 0;
            ///< Remove records that are flagged as erased.
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

            virtual int getColumns() const;

            virtual QVariant getData (int index, int column) const;

            virtual void setData (int index, int column, const QVariant& data);

            virtual std::string getTitle (int column) const;

            virtual bool isEditable (int column) const;

            virtual void merge();
            ///< Merge modified into base.

            virtual void purge();
            ///< Remove records that are flagged as erased.

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
        std::string id;

        std::transform (record.mId.begin(), record.mId.end(), std::back_inserter (id),
            (int(*)(int)) std::tolower);

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
    std::string IdCollection<ESXRecordT>::getTitle (int column) const
    {
        return mColumns.at (column)->mTitle;
    }

    template<typename ESXRecordT>
    bool IdCollection<ESXRecordT>::isEditable (int column) const
    {
        return mColumns.at (column)->isEditable();
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
}

#endif
