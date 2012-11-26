#ifndef CSM_WOLRD_RECORD_H
#define CSM_WOLRD_RECORD_H

#include <stdexcept>

namespace CSMWorld
{
    template <typename ESXRecordT>
    struct Record
    {
        enum state
        {
            State_BaseOnly, // defined in base only
            State_Modified, // exists in base, but has been modified
            State_ModifiedOnly, // newly created in modified
            State_Deleted // exists in base, but has been deleted
        };

        ESXRecordT mBase;
        ESXRecordT mModified;
        state mState;

        bool isDeleted() const;

        bool isModified() const;

        const ESXRecordT& get() const;
        ///< Throws an exception, if the record is deleted.

        ESXRecordT& get();
        ///< Throws an exception, if the record is deleted.

        void setModified (const ESXRecordT& modified);
    };

    template <typename ESXRecordT>
    bool Record<ESXRecordT>::isDeleted() const
    {
        return mState==State_Deleted;
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
    ESXRecordT& Record<ESXRecordT>::get()
    {
        if (isDeleted())
            throw std::logic_error ("attempt to access a deleted record");

        return mState==State_BaseOnly ? mBase : mModified;
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::setModified (const ESXRecordT& modified)
    {
        mModified = modified;

        if (mState!=State_ModifiedOnly)
            mState = State_Modified;
    }
}

#endif
