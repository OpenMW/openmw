#ifndef CSM_WOLRD_RECORD_H
#define CSM_WOLRD_RECORD_H

#include <stdexcept>

namespace CSMWorld
{
    struct RecordBase
    {
        enum State
        {
            State_BaseOnly = 0, // defined in base only
            State_Modified = 1, // exists in base, but has been modified
            State_ModifiedOnly = 2, // newly created in modified
            State_Deleted = 3, // exists in base, but has been deleted
            State_Erased = 4 // does not exist at all (we mostly treat that the same way as deleted)
        };

        State mState;

        virtual ~RecordBase();

        virtual RecordBase *clone() const = 0;

        bool isDeleted() const;

        bool isErased() const;

        bool isModified() const;
    };

    template <typename ESXRecordT>
    struct Record : public RecordBase
    {
        ESXRecordT mBase;
        ESXRecordT mModified;

        virtual RecordBase *clone() const;

        const ESXRecordT& get() const;
        ///< Throws an exception, if the record is deleted.

        const ESXRecordT& getBase() const;
        ///< Throws an exception, if the record is deleted. Returns modified, if there is no base.

        void setModified (const ESXRecordT& modified);
        ///< Throws an exception, if the record is deleted.

        void merge();
        ///< Merge modified into base.
    };

    template <typename ESXRecordT>
    RecordBase *Record<ESXRecordT>::clone() const
    {
        return new Record<ESXRecordT> (*this);
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::get() const
    {
        if (mState==State_Erased)
            throw std::logic_error ("attempt to access a deleted record");

        return mState==State_BaseOnly || mState==State_Deleted ? mBase : mModified;
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::getBase() const
    {
        if (mState==State_Erased)
            throw std::logic_error ("attempt to access a deleted record");

        return mState==State_ModifiedOnly ? mModified : mBase;
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::setModified (const ESXRecordT& modified)
    {
        if (mState==State_Erased)
            throw std::logic_error ("attempt to modify a deleted record");

        mModified = modified;
        mState = State_Modified;
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::merge()
    {
        if (isModified())
        {
            mBase = mModified;
            mState = State_BaseOnly;
        }
        else if (mState==State_Deleted)
        {
            mState = State_Erased;
        }
    }
}

#endif
