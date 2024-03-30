#ifndef CSM_WOLRD_RECORD_H
#define CSM_WOLRD_RECORD_H

#include <memory>
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

        explicit RecordBase(State state)
            : mState(state)
        {
        }

        virtual ~RecordBase() = default;

        virtual std::unique_ptr<RecordBase> clone() const = 0;

        virtual std::unique_ptr<RecordBase> modifiedCopy() const = 0;

        virtual void assign(const RecordBase& record) = 0;
        ///< Will throw an exception if the types don't match.

        bool isDeleted() const;

        bool isErased() const;

        bool isModified() const;
    };

    template <typename ESXRecordT>
    struct Record : public RecordBase
    {
        ESXRecordT mBase;
        ESXRecordT mModified;

        Record();

        Record(State state, const ESXRecordT* base = 0, const ESXRecordT* modified = 0);

        std::unique_ptr<RecordBase> clone() const override;

        std::unique_ptr<RecordBase> modifiedCopy() const override;

        void assign(const RecordBase& record) override;

        const ESXRecordT& get() const;
        ///< Throws an exception, if the record is deleted.

        ESXRecordT& get();
        ///< Throws an exception, if the record is deleted.

        const ESXRecordT& getBase() const;
        ///< Throws an exception, if the record is deleted. Returns modified, if there is no base.

        void setModified(const ESXRecordT& modified);
        ///< Throws an exception, if the record is deleted.

        void merge();
        ///< Merge modified into base.
    };

    template <typename ESXRecordT>
    Record<ESXRecordT>::Record()
        : RecordBase(State_BaseOnly)
        , mBase()
        , mModified()
    {
    }

    template <typename ESXRecordT>
    Record<ESXRecordT>::Record(State state, const ESXRecordT* base, const ESXRecordT* modified)
        : RecordBase(state)
        , mBase(base == nullptr ? ESXRecordT{} : *base)
        , mModified(modified == nullptr ? ESXRecordT{} : *modified)
    {
    }

    template <typename ESXRecordT>
    std::unique_ptr<RecordBase> Record<ESXRecordT>::modifiedCopy() const
    {
        return std::make_unique<Record<ESXRecordT>>(Record<ESXRecordT>(State_ModifiedOnly, nullptr, &(this->get())));
    }

    template <typename ESXRecordT>
    std::unique_ptr<RecordBase> Record<ESXRecordT>::clone() const
    {
        return std::make_unique<Record<ESXRecordT>>(Record<ESXRecordT>(*this));
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::assign(const RecordBase& record)
    {
        *this = dynamic_cast<const Record<ESXRecordT>&>(record);
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::get() const
    {
        if (mState == State_Erased)
            throw std::logic_error("attempt to access a deleted record");

        return mState == State_BaseOnly || mState == State_Deleted ? mBase : mModified;
    }

    template <typename ESXRecordT>
    ESXRecordT& Record<ESXRecordT>::get()
    {
        if (mState == State_Erased)
            throw std::logic_error("attempt to access a deleted record");

        return mState == State_BaseOnly || mState == State_Deleted ? mBase : mModified;
    }

    template <typename ESXRecordT>
    const ESXRecordT& Record<ESXRecordT>::getBase() const
    {
        if (mState == State_Erased)
            throw std::logic_error("attempt to access a deleted record");

        return mState == State_ModifiedOnly ? mModified : mBase;
    }

    template <typename ESXRecordT>
    void Record<ESXRecordT>::setModified(const ESXRecordT& modified)
    {
        if (mState == State_Erased)
            throw std::logic_error("attempt to modify a deleted record");

        mModified = modified;

        if (mState != State_ModifiedOnly)
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
        else if (mState == State_Deleted)
        {
            mState = State_Erased;
        }
    }
}

#endif
