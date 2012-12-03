#ifndef CSM_WOLRD_RECORD_H
#define CSM_WOLRD_RECORD_H

#include <stdexcept>

namespace CSMWorld
{
    template <typename ESXRecordT>
    struct Record
    {
        enum State
        {
            State_BaseOnly, // defined in base only
            State_Modified, // exists in base, but has been modified
            State_ModifiedOnly, // newly created in modified
            State_Deleted, // exists in base, but has been deleted
            State_Erased // does not exist at all (we mostly treat that the same way as deleted)
        };

        ESXRecordT mBase;
        ESXRecordT mModified;
        State mState;

        bool isDeleted() const;

        bool isErased() const;

        bool isModified() const;

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
    bool Record<ESXRecordT>::isDeleted() const
    {
        return mState==State_Deleted || mState==State_Erased;
    }

    template <typename ESXRecordT>
    bool Record<ESXRecordT>::isErased() const
    {
        return mState==State_Erased;
    }

    template <typename ESXRecordT>
    bool Record<ESXRecordT>::isModified() const
    {
        return mState==State_Modified || mState==State_ModifiedOnly;
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::get() const
    {
        if (isDeleted())
            throw std::logic_error ("attempt to access a deleted record");

        return mState==State_BaseOnly ? mBase : mModified;
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::getBase() const
    {
        if (isDeleted())
            throw std::logic_error ("attempt to access a deleted record");

        return mState==State_ModifiedOnly ? mModified : mBase;
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::setModified (const ESXRecordT& modified)
    {
        if (isDeleted())
            throw std::logic_error ("attempt to modify a deleted record");

        mModified = modified;

        if (mState!=State_ModifiedOnly)
            mState = mBase==mModified ? State_BaseOnly : State_Modified;
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
