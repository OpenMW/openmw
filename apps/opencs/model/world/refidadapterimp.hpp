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
        const RefIdColumn *mType;
    };

    /// \brief Base adapter for all refereceable record types
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

            UniversalId::Type getType() const;
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

        if (column==mBase.mType)
            return static_cast<int> (mType);

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

    template<typename RecordT>
    UniversalId::Type BaseRefIdAdapter<RecordT>::getType() const
    {
        return mType;
    }


    struct ModelColumns : public BaseColumns
    {
        const RefIdColumn *mModel;

        ModelColumns (const BaseColumns& base) : BaseColumns (base) {}
    };

    /// \brief Adapter for IDs with models (all but levelled lists)
    template<typename RecordT>
    class ModelRefIdAdapter : public BaseRefIdAdapter<RecordT>
    {
            ModelColumns mModel;

        public:

            ModelRefIdAdapter (UniversalId::Type type, const ModelColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    ModelRefIdAdapter<RecordT>::ModelRefIdAdapter (UniversalId::Type type, const ModelColumns& columns)
    : BaseRefIdAdapter<RecordT> (type, columns), mModel (columns)
    {}

    template<typename RecordT>
    QVariant ModelRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mModel.mModel)
            return QString::fromUtf8 (record.get().mModel.c_str());

        return BaseRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void ModelRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mModel.mModel)
            record.get().mModel = value.toString().toUtf8().constData();
        else
            BaseRefIdAdapter<RecordT>::setData (column, data, index, value);
    }

    struct NameColumns : public ModelColumns
    {
        const RefIdColumn *mName;
        const RefIdColumn *mScript;

        NameColumns (const ModelColumns& base) : ModelColumns (base) {}
    };

    /// \brief Adapter for IDs with names (all but levelled lists and statics)
    template<typename RecordT>
    class NameRefIdAdapter : public ModelRefIdAdapter<RecordT>
    {
            NameColumns mName;

        public:

            NameRefIdAdapter (UniversalId::Type type, const NameColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    NameRefIdAdapter<RecordT>::NameRefIdAdapter (UniversalId::Type type, const NameColumns& columns)
    : ModelRefIdAdapter<RecordT> (type, columns), mName (columns)
    {}

    template<typename RecordT>
    QVariant NameRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mName.mName)
            return QString::fromUtf8 (record.get().mName.c_str());

        if (column==mName.mScript)
            return QString::fromUtf8 (record.get().mScript.c_str());

        return ModelRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void NameRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mName.mName)
            record.get().mName = value.toString().toUtf8().constData();
        else if (column==mName.mScript)
            record.get().mScript = value.toString().toUtf8().constData();
        else
            ModelRefIdAdapter<RecordT>::setData (column, data, index, value);
    }

    struct InventoryColumns : public NameColumns
    {
        const RefIdColumn *mIcon;
        const RefIdColumn *mWeight;
        const RefIdColumn *mValue;

        InventoryColumns (const NameColumns& base) : NameColumns (base) {}
    };

    /// \brief Adapter for IDs with names (all but levelled lists and statics)
    template<typename RecordT>
    class InventoryRefIdAdapter : public NameRefIdAdapter<RecordT>
    {
            InventoryColumns mName;

        public:

            InventoryRefIdAdapter (UniversalId::Type type, const InventoryColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    InventoryRefIdAdapter<RecordT>::InventoryRefIdAdapter (UniversalId::Type type,
        const InventoryColumns& columns)
    : NameRefIdAdapter<RecordT> (type, columns), mName (columns)
    {}

    template<typename RecordT>
    QVariant InventoryRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mName.mIcon)
            return QString::fromUtf8 (record.get().mIcon.c_str());

        if (column==mName.mWeight)
            return record.get().mData.mWeight;

        if (column==mName.mValue)
            return record.get().mData.mValue;

        return NameRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void InventoryRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mName.mIcon)
            record.get().mIcon = value.toString().toUtf8().constData();
        else if (column==mName.mWeight)
            record.get().mData.mWeight = value.toFloat();
        else if (column==mName.mValue)
            record.get().mData.mValue = value.toInt();
        else
            NameRefIdAdapter<RecordT>::setData (column, data, index, value);
    }
}

#endif
