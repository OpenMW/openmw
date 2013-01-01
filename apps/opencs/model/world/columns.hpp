#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include "columnbase.hpp"

namespace CSMWorld
{
    template<typename ESXRecordT>
    struct FloatValueColumn : public Column<ESXRecordT>
    {
        FloatValueColumn() : Column<ESXRecordT> ("Value") {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mValue;
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            ESXRecordT base = record.getBase();
            base.mValue = data.toFloat();
            record.setModified (base);
        }

        virtual bool isEditable() const
        {
            return true;
        }
    };

    template<typename ESXRecordT>
    struct StringIdColumn : public Column<ESXRecordT>
    {
        StringIdColumn() : Column<ESXRecordT> ("ID") {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return record.get().mId.c_str();
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct RecordStateColumn : public Column<ESXRecordT>
    {
        RecordStateColumn() : Column<ESXRecordT> ("*") {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            if (record.mState==Record<ESXRecordT>::State_Erased)
                return static_cast<int> (Record<ESXRecordT>::State_Deleted);

            return static_cast<int> (record.mState);
        }

        virtual void set (Record<ESXRecordT>& record, const QVariant& data)
        {
            record.mState = static_cast<RecordBase::State> (data.toInt());
        }

        virtual bool isEditable() const
        {
            return true;
        }

        virtual bool isUserEditable() const
        {
            return false;
        }
    };

    template<typename ESXRecordT>
    struct FixedRecordTypeColumn : public Column<ESXRecordT>
    {
        int mType;

        FixedRecordTypeColumn (int type) : Column<ESXRecordT> ("Type", 0), mType (type) {}

        virtual QVariant get (const Record<ESXRecordT>& record) const
        {
            return mType;
        }

        virtual bool isEditable() const
        {
            return false;
        }
    };
}

#endif