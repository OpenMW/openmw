#ifndef CSM_WOLRD_REFIDADAPTERIMP_H
#define CSM_WOLRD_REFIDADAPTERIMP_H

#include <QVariant>

#include "record.hpp"
#include "refiddata.hpp"
#include "universalid.hpp"
#include "refidadapter.hpp"

namespace CSMWorld
{
    struct BaseColumns
    {
        const RefIdColumn *mId;
        const RefIdColumn *mModified;
    };

    template<typename RecordT>
    class BaseRefIdAdapter : public RefIdAdapter
    {
            UniversalId::Type mType;
            BaseColumns mBase;

        public:

            BaseRefIdAdapter (UniversalId::Type type, const BaseColumns& base);

            virtual std::string getId (const RecordBase& record) const;

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    BaseRefIdAdapter<RecordT>::BaseRefIdAdapter (UniversalId::Type type, const BaseColumns& base)
    : mType (type), mBase (base)
    {}

    template<typename RecordT>
    std::string BaseRefIdAdapter<RecordT>::getId (const RecordBase& record) const
    {
        return dynamic_cast<const Record<RecordT>&> (record).get().mId;
    }

    template<typename RecordT>
    QVariant BaseRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, mType)));

        if (column==mBase.mId)
            return QString::fromUtf8 (record.get().mId.c_str());

        if (column==mBase.mModified)
        {
            if (record.mState==Record<RecordT>::State_Erased)
                return static_cast<int> (Record<RecordT>::State_Deleted);

            return static_cast<int> (record.mState);
        }

        return QVariant();
    }

    template<typename RecordT>
    void BaseRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, mType)));

        if (column==mBase.mModified)
            record.mState = static_cast<RecordBase::State> (value.toInt());
    }
}

#endif
