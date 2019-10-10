#ifndef CSM_WOLRD_REFIDADAPTERIMP_H
#define CSM_WOLRD_REFIDADAPTERIMP_H

#include <map>

#include <QVariant>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/loadcrea.hpp>

#include "columnbase.hpp"
#include "record.hpp"
#include "refiddata.hpp"
#include "universalid.hpp"
#include "refidadapter.hpp"
#include "nestedtablewrapper.hpp"

namespace CSMWorld
{
    struct BaseColumns
    {
        const RefIdColumn *mId;
        const RefIdColumn *mModified;
        const RefIdColumn *mType;
    };

    /// \brief Base adapter for all refereceable record types
    /// Adapters that can handle nested tables, needs to return valid qvariant for parent columns
    template<typename RecordT>
    class BaseRefIdAdapter : public RefIdAdapter
    {
            UniversalId::Type mType;
            BaseColumns mBase;

        public:

            BaseRefIdAdapter (UniversalId::Type type, const BaseColumns& base);

            virtual std::string getId (const RecordBase& record) const;

            virtual void setId (RecordBase& record, const std::string& id);

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
    void BaseRefIdAdapter<RecordT>::setId (RecordBase& record, const std::string& id)
    {
        (dynamic_cast<Record<RecordT>&> (record).get().mId) = id;
    }

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

        RecordT record2 = record.get();
        if (column==mModel.mModel)
            record2.mModel = value.toString().toUtf8().constData();
        else
        {
            BaseRefIdAdapter<RecordT>::setData (column, data, index, value);
            return;
        }

        record.setModified(record2);
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

        RecordT record2 = record.get();
        if (column==mName.mName)
            record2.mName = value.toString().toUtf8().constData();
        else if (column==mName.mScript)
            record2.mScript = value.toString().toUtf8().constData();
        else
        {
            ModelRefIdAdapter<RecordT>::setData (column, data, index, value);
            return;
        }

        record.setModified(record2);
    }

    struct InventoryColumns : public NameColumns
    {
        const RefIdColumn *mIcon;
        const RefIdColumn *mWeight;
        const RefIdColumn *mValue;

        InventoryColumns (const NameColumns& base) : NameColumns (base) {}
    };

    /// \brief Adapter for IDs that can go into an inventory
    template<typename RecordT>
    class InventoryRefIdAdapter : public NameRefIdAdapter<RecordT>
    {
            InventoryColumns mInventory;

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
    : NameRefIdAdapter<RecordT> (type, columns), mInventory (columns)
    {}

    template<typename RecordT>
    QVariant InventoryRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mInventory.mIcon)
            return QString::fromUtf8 (record.get().mIcon.c_str());

        if (column==mInventory.mWeight)
            return record.get().mData.mWeight;

        if (column==mInventory.mValue)
            return record.get().mData.mValue;

        return NameRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void InventoryRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        RecordT record2 = record.get();
        if (column==mInventory.mIcon)
            record2.mIcon = value.toString().toUtf8().constData();
        else if (column==mInventory.mWeight)
            record2.mData.mWeight = value.toFloat();
        else if (column==mInventory.mValue)
            record2.mData.mValue = value.toInt();
        else
        {
            NameRefIdAdapter<RecordT>::setData (column, data, index, value);
            return;
        }

        record.setModified(record2);
    }

    struct PotionColumns : public InventoryColumns
    {
        const RefIdColumn *mEffects;

        PotionColumns (const InventoryColumns& columns);
    };

    class PotionRefIdAdapter : public InventoryRefIdAdapter<ESM::Potion>
    {
            PotionColumns mColumns;
            const RefIdColumn *mAutoCalc;

        public:

            PotionRefIdAdapter (const PotionColumns& columns, const RefIdColumn *autoCalc);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct IngredientColumns : public InventoryColumns
    {
        const RefIdColumn *mEffects;

        IngredientColumns (const InventoryColumns& columns);
    };

    class IngredientRefIdAdapter : public InventoryRefIdAdapter<ESM::Ingredient>
    {
            IngredientColumns mColumns;

        public:

            IngredientRefIdAdapter (const IngredientColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class IngredEffectRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        IngredEffectRefIdAdapter (const IngredEffectRefIdAdapter&);
        IngredEffectRefIdAdapter& operator= (const IngredEffectRefIdAdapter&);

    public:

        IngredEffectRefIdAdapter();

        virtual ~IngredEffectRefIdAdapter();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    struct EnchantableColumns : public InventoryColumns
    {
        const RefIdColumn *mEnchantment;
        const RefIdColumn *mEnchantmentPoints;

        EnchantableColumns (const InventoryColumns& base) : InventoryColumns (base) {}
    };

    /// \brief Adapter for enchantable IDs
    template<typename RecordT>
    class EnchantableRefIdAdapter : public InventoryRefIdAdapter<RecordT>
    {
            EnchantableColumns mEnchantable;

        public:

            EnchantableRefIdAdapter (UniversalId::Type type, const EnchantableColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    EnchantableRefIdAdapter<RecordT>::EnchantableRefIdAdapter (UniversalId::Type type,
        const EnchantableColumns& columns)
    : InventoryRefIdAdapter<RecordT> (type, columns), mEnchantable (columns)
    {}

    template<typename RecordT>
    QVariant EnchantableRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mEnchantable.mEnchantment)
            return QString::fromUtf8 (record.get().mEnchant.c_str());

        if (column==mEnchantable.mEnchantmentPoints)
            return static_cast<int> (record.get().mData.mEnchant);

        return InventoryRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void EnchantableRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data,
        int index, const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        RecordT record2 = record.get();
        if (column==mEnchantable.mEnchantment)
            record2.mEnchant = value.toString().toUtf8().constData();
        else if (column==mEnchantable.mEnchantmentPoints)
            record2.mData.mEnchant = value.toInt();
        else
        {
            InventoryRefIdAdapter<RecordT>::setData (column, data, index, value);
            return;
        }

        record.setModified(record2);
    }

    struct ToolColumns : public InventoryColumns
    {
        const RefIdColumn *mQuality;
        const RefIdColumn *mUses;

        ToolColumns (const InventoryColumns& base) : InventoryColumns (base) {}
    };

    /// \brief Adapter for tools with limited uses IDs (lockpick, repair, probes)
    template<typename RecordT>
    class ToolRefIdAdapter : public InventoryRefIdAdapter<RecordT>
    {
            ToolColumns mTools;

        public:

            ToolRefIdAdapter (UniversalId::Type type, const ToolColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    ToolRefIdAdapter<RecordT>::ToolRefIdAdapter (UniversalId::Type type, const ToolColumns& columns)
    : InventoryRefIdAdapter<RecordT> (type, columns), mTools (columns)
    {}

    template<typename RecordT>
    QVariant ToolRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mTools.mQuality)
            return record.get().mData.mQuality;

        if (column==mTools.mUses)
            return record.get().mData.mUses;

        return InventoryRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void ToolRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data,
        int index, const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        RecordT record2 = record.get();
        if (column==mTools.mQuality)
            record2.mData.mQuality = value.toFloat();
        else if (column==mTools.mUses)
            record2.mData.mUses = value.toInt();
        else
        {
            InventoryRefIdAdapter<RecordT>::setData (column, data, index, value);
            return;
        }

        record.setModified(record2);
    }

    struct ActorColumns : public NameColumns
    {
        const RefIdColumn *mHello;
        const RefIdColumn *mFlee;
        const RefIdColumn *mFight;
        const RefIdColumn *mAlarm;
        const RefIdColumn *mInventory;
        const RefIdColumn *mSpells;
        const RefIdColumn *mDestinations;
        const RefIdColumn *mAiPackages;
        std::map<const RefIdColumn *, unsigned int> mServices;

        ActorColumns (const NameColumns& base) : NameColumns (base) {}
    };

    /// \brief Adapter for actor IDs (handles common AI functionality)
    template<typename RecordT>
    class ActorRefIdAdapter : public NameRefIdAdapter<RecordT>
    {
            ActorColumns mActors;

        public:

            ActorRefIdAdapter (UniversalId::Type type, const ActorColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    ActorRefIdAdapter<RecordT>::ActorRefIdAdapter (UniversalId::Type type,
        const ActorColumns& columns)
    : NameRefIdAdapter<RecordT> (type, columns), mActors (columns)
    {}

    template<typename RecordT>
    QVariant ActorRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        const Record<RecordT>& record = static_cast<const Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        if (column==mActors.mHello)
            return record.get().mAiData.mHello;

        if (column==mActors.mFlee)
            return record.get().mAiData.mFlee;

        if (column==mActors.mFight)
            return record.get().mAiData.mFight;

        if (column==mActors.mAlarm)
            return record.get().mAiData.mAlarm;

        if (column==mActors.mInventory)
            return QVariant::fromValue(ColumnBase::TableEdit_Full);

        if (column==mActors.mSpells)
            return QVariant::fromValue(ColumnBase::TableEdit_Full);

        if (column==mActors.mDestinations)
            return QVariant::fromValue(ColumnBase::TableEdit_Full);

        if (column==mActors.mAiPackages)
            return QVariant::fromValue(ColumnBase::TableEdit_Full);

        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mActors.mServices.find (column);

        if (iter!=mActors.mServices.end())
            return (record.get().mAiData.mServices & iter->second)!=0;

        return NameRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void ActorRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        Record<RecordT>& record = static_cast<Record<RecordT>&> (
            data.getRecord (RefIdData::LocalIndex (index, BaseRefIdAdapter<RecordT>::getType())));

        RecordT record2 = record.get();
        if (column==mActors.mHello)
            record2.mAiData.mHello = value.toInt();
        else if (column==mActors.mFlee) // Flee, Fight and Alarm ratings are probabilities.
            record2.mAiData.mFlee = std::min(100, value.toInt());
        else if (column==mActors.mFight)
            record2.mAiData.mFight = std::min(100, value.toInt());
        else if (column==mActors.mAlarm)
            record2.mAiData.mAlarm = std::min(100, value.toInt());
        else
        {
            typename std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
                mActors.mServices.find (column);
            if (iter!=mActors.mServices.end())
            {
                if (value.toInt()!=0)
                    record2.mAiData.mServices |= iter->second;
                else
                    record2.mAiData.mServices &= ~iter->second;
            }
            else
            {
                NameRefIdAdapter<RecordT>::setData (column, data, index, value);
                return;
            }
        }

        record.setModified(record2);
    }

    class ApparatusRefIdAdapter : public InventoryRefIdAdapter<ESM::Apparatus>
    {
            const RefIdColumn *mType;
            const RefIdColumn *mQuality;

        public:

            ApparatusRefIdAdapter (const InventoryColumns& columns, const RefIdColumn *type,
                const RefIdColumn *quality);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class ArmorRefIdAdapter : public EnchantableRefIdAdapter<ESM::Armor>
    {
            const RefIdColumn *mType;
            const RefIdColumn *mHealth;
            const RefIdColumn *mArmor;
            const RefIdColumn *mPartRef;

        public:

            ArmorRefIdAdapter (const EnchantableColumns& columns, const RefIdColumn *type,
                const RefIdColumn *health, const RefIdColumn *armor, const RefIdColumn *partRef);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class BookRefIdAdapter : public EnchantableRefIdAdapter<ESM::Book>
    {
            const RefIdColumn *mBookType;
            const RefIdColumn *mSkill;
            const RefIdColumn *mText;

        public:

            BookRefIdAdapter (const EnchantableColumns& columns, const RefIdColumn *bookType,
                const RefIdColumn *skill, const RefIdColumn *text);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class ClothingRefIdAdapter : public EnchantableRefIdAdapter<ESM::Clothing>
    {
            const RefIdColumn *mType;
            const RefIdColumn *mPartRef;

        public:

            ClothingRefIdAdapter (const EnchantableColumns& columns,
                    const RefIdColumn *type, const RefIdColumn *partRef);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class ContainerRefIdAdapter : public NameRefIdAdapter<ESM::Container>
    {
            const RefIdColumn *mWeight;
            const RefIdColumn *mOrganic;
            const RefIdColumn *mRespawn;
            const RefIdColumn *mContent;

        public:

            ContainerRefIdAdapter (const NameColumns& columns, const RefIdColumn *weight,
                                   const RefIdColumn *organic, const RefIdColumn *respawn, const RefIdColumn *content);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index) const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                                  const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct CreatureColumns : public ActorColumns
    {
        std::map<const RefIdColumn *, unsigned int> mFlags;
        const RefIdColumn *mType;
        const RefIdColumn *mScale;
        const RefIdColumn *mOriginal;
        const RefIdColumn *mAttributes;
        const RefIdColumn *mAttacks;
        const RefIdColumn *mMisc;
        const RefIdColumn *mBloodType;

        CreatureColumns (const ActorColumns& actorColumns);
    };

    class CreatureRefIdAdapter : public ActorRefIdAdapter<ESM::Creature>
    {
            CreatureColumns mColumns;

        public:

            CreatureRefIdAdapter (const CreatureColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class DoorRefIdAdapter : public NameRefIdAdapter<ESM::Door>
    {
            const RefIdColumn *mOpenSound;
            const RefIdColumn *mCloseSound;

        public:

            DoorRefIdAdapter (const NameColumns& columns, const RefIdColumn *openSound,
                const RefIdColumn *closeSound);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct LightColumns : public InventoryColumns
    {
        const RefIdColumn *mTime;
        const RefIdColumn *mRadius;
        const RefIdColumn *mColor;
        const RefIdColumn *mSound;
        const RefIdColumn *mEmitterType;
        std::map<const RefIdColumn *, unsigned int> mFlags;

        LightColumns (const InventoryColumns& columns);
    };

    class LightRefIdAdapter : public InventoryRefIdAdapter<ESM::Light>
    {
            LightColumns mColumns;

        public:

            LightRefIdAdapter (const LightColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class MiscRefIdAdapter : public InventoryRefIdAdapter<ESM::Miscellaneous>
    {
            const RefIdColumn *mKey;

        public:

            MiscRefIdAdapter (const InventoryColumns& columns, const RefIdColumn *key);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct NpcColumns : public ActorColumns
    {
        std::map<const RefIdColumn *, unsigned int> mFlags;
        const RefIdColumn *mRace;
        const RefIdColumn *mClass;
        const RefIdColumn *mFaction;
        const RefIdColumn *mHair;
        const RefIdColumn *mHead;
        const RefIdColumn *mAttributes; // depends on npc type
        const RefIdColumn *mSkills;     // depends on npc type
        const RefIdColumn *mMisc;       // may depend on npc type, e.g. FactionID
        const RefIdColumn *mBloodType;
        const RefIdColumn *mGender;

        NpcColumns (const ActorColumns& actorColumns);
    };

    class NpcRefIdAdapter : public ActorRefIdAdapter<ESM::NPC>
    {
            NpcColumns mColumns;

        public:

            NpcRefIdAdapter (const NpcColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct WeaponColumns : public EnchantableColumns
    {
        const RefIdColumn *mType;
        const RefIdColumn *mHealth;
        const RefIdColumn *mSpeed;
        const RefIdColumn *mReach;
        const RefIdColumn *mChop[2];
        const RefIdColumn *mSlash[2];
        const RefIdColumn *mThrust[2];
        std::map<const RefIdColumn *, unsigned int> mFlags;

        WeaponColumns (const EnchantableColumns& columns);
    };

    class WeaponRefIdAdapter : public EnchantableRefIdAdapter<ESM::Weapon>
    {
            WeaponColumns mColumns;

        public:

            WeaponRefIdAdapter (const WeaponColumns& columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };


    class NestedRefIdAdapterBase;

    class NpcAttributesRefIdAdapter : public NestedRefIdAdapterBase
    {
    public:

        NpcAttributesRefIdAdapter ();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    class NpcSkillsRefIdAdapter : public NestedRefIdAdapterBase
    {
    public:

        NpcSkillsRefIdAdapter ();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    class NpcMiscRefIdAdapter : public NestedRefIdAdapterBase
    {
        NpcMiscRefIdAdapter (const NpcMiscRefIdAdapter&);
        NpcMiscRefIdAdapter& operator= (const NpcMiscRefIdAdapter&);

    public:

        NpcMiscRefIdAdapter ();
        virtual ~NpcMiscRefIdAdapter();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    class CreatureAttributesRefIdAdapter : public NestedRefIdAdapterBase
    {
    public:

        CreatureAttributesRefIdAdapter ();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    class CreatureAttackRefIdAdapter : public NestedRefIdAdapterBase
    {
    public:

        CreatureAttackRefIdAdapter ();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    class CreatureMiscRefIdAdapter : public NestedRefIdAdapterBase
    {
        CreatureMiscRefIdAdapter (const CreatureMiscRefIdAdapter&);
        CreatureMiscRefIdAdapter& operator= (const CreatureMiscRefIdAdapter&);

    public:

        CreatureMiscRefIdAdapter ();
        virtual ~CreatureMiscRefIdAdapter();

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const;

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const;

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const;

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const;

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const;

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const;

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const;

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const;
    };

    template<typename ESXRecordT>
    class EffectsListAdapter;

    template<typename ESXRecordT>
    class EffectsRefIdAdapter : public EffectsListAdapter<ESXRecordT>, public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        EffectsRefIdAdapter (const EffectsRefIdAdapter&);
        EffectsRefIdAdapter& operator= (const EffectsRefIdAdapter&);

    public:

        EffectsRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~EffectsRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            EffectsListAdapter<ESXRecordT>::addRow(record, position);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            EffectsListAdapter<ESXRecordT>::removeRow(record, rowToRemove);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            EffectsListAdapter<ESXRecordT>::setTable(record, nestedTable);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            return EffectsListAdapter<ESXRecordT>::table(record);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            return EffectsListAdapter<ESXRecordT>::getData(record, subRowIndex, subColIndex);
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            EffectsListAdapter<ESXRecordT>::setData(record, value, subRowIndex, subColIndex);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            const Record<ESXRecordT> record; // not used, just a dummy
            return EffectsListAdapter<ESXRecordT>::getColumnsCount(record);
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            return EffectsListAdapter<ESXRecordT>::getRowsCount(record);
        }
    };

    template <typename ESXRecordT>
    class NestedInventoryRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        NestedInventoryRefIdAdapter (const NestedInventoryRefIdAdapter&);
        NestedInventoryRefIdAdapter& operator= (const NestedInventoryRefIdAdapter&);

    public:

        NestedInventoryRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~NestedInventoryRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT container = record.get();

            std::vector<ESM::ContItem>& list = container.mInventory.mList;

            ESM::ContItem newRow = ESM::ContItem();

            if (position >= (int)list.size())
                list.push_back(newRow);
            else
                list.insert(list.begin()+position, newRow);

            record.setModified (container);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT container = record.get();

            std::vector<ESM::ContItem>& list = container.mInventory.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (container);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT container = record.get();

            container.mInventory.mList =
                static_cast<const NestedTableWrapper<std::vector<typename ESM::ContItem> >&>(nestedTable).mNestedTable;

            record.setModified (container);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESM::ContItem> >(record.get().mInventory.mList);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<ESM::ContItem>& list = record.get().mInventory.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const ESM::ContItem& content = list.at(subRowIndex);

            switch (subColIndex)
            {
                case 0: return QString::fromUtf8(content.mItem.c_str());
                case 1: return content.mCount;
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT container = record.get();
            std::vector<ESM::ContItem>& list = container.mInventory.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            switch(subColIndex)
            {
                case 0:
                    list.at(subRowIndex).mItem.assign(std::string(value.toString().toUtf8().constData()));
                    break;

                case 1:
                    list.at(subRowIndex).mCount = value.toInt();
                    break;

                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }

            record.setModified (container);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 2;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mInventory.mList.size());
        }
    };

    template <typename ESXRecordT>
    class NestedSpellRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        NestedSpellRefIdAdapter (const NestedSpellRefIdAdapter&);
        NestedSpellRefIdAdapter& operator= (const NestedSpellRefIdAdapter&);

    public:

        NestedSpellRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~NestedSpellRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT caster = record.get();

            std::vector<std::string>& list = caster.mSpells.mList;

            std::string newString;

            if (position >= (int)list.size())
                list.push_back(newString);
            else
                list.insert(list.begin()+position, newString);

            record.setModified (caster);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT caster = record.get();

            std::vector<std::string>& list = caster.mSpells.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (caster);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT caster = record.get();

            caster.mSpells.mList =
                static_cast<const NestedTableWrapper<std::vector<typename std::string> >&>(nestedTable).mNestedTable;

            record.setModified (caster);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename std::string> >(record.get().mSpells.mList);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<std::string>& list = record.get().mSpells.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const std::string& content = list.at(subRowIndex);

            if (subColIndex == 0)
                return QString::fromUtf8(content.c_str());
            else
                throw std::runtime_error("Trying to access non-existing column in the nested table!");
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT caster = record.get();
            std::vector<std::string>& list = caster.mSpells.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            if (subColIndex == 0)
                list.at(subRowIndex) = std::string(value.toString().toUtf8());
            else
                throw std::runtime_error("Trying to access non-existing column in the nested table!");

            record.setModified (caster);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 1;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mSpells.mList.size());
        }
    };

    template <typename ESXRecordT>
    class NestedTravelRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        NestedTravelRefIdAdapter (const NestedTravelRefIdAdapter&);
        NestedTravelRefIdAdapter& operator= (const NestedTravelRefIdAdapter&);

    public:

        NestedTravelRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~NestedTravelRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT traveller = record.get();

            std::vector<ESM::Transport::Dest>& list = traveller.mTransport.mList;

            ESM::Position newPos;
            for (unsigned i = 0; i < 3; ++i)
            {
                newPos.pos[i] = 0;
                newPos.rot[i] = 0;
            }

            ESM::Transport::Dest newRow;
            newRow.mPos = newPos;
            newRow.mCellName = "";

            if (position >= (int)list.size())
                list.push_back(newRow);
            else
                list.insert(list.begin()+position, newRow);

            record.setModified (traveller);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT traveller = record.get();

            std::vector<ESM::Transport::Dest>& list = traveller.mTransport.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (traveller);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT traveller = record.get();

            traveller.mTransport.mList =
                static_cast<const NestedTableWrapper<std::vector<typename ESM::Transport::Dest> >&>(nestedTable).mNestedTable;

            record.setModified (traveller);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESM::Transport::Dest> >(record.get().mTransport.mList);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<ESM::Transport::Dest>& list = record.get().mTransport.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const ESM::Transport::Dest& content = list.at(subRowIndex);

            switch (subColIndex)
            {
                case 0: return QString::fromUtf8(content.mCellName.c_str());
                case 1: return content.mPos.pos[0];
                case 2: return content.mPos.pos[1];
                case 3: return content.mPos.pos[2];
                case 4: return content.mPos.rot[0];
                case 5: return content.mPos.rot[1];
                case 6: return content.mPos.rot[2];
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT traveller = record.get();
            std::vector<ESM::Transport::Dest>& list = traveller.mTransport.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            switch(subColIndex)
            {
                case 0: list.at(subRowIndex).mCellName = std::string(value.toString().toUtf8().constData()); break;
                case 1: list.at(subRowIndex).mPos.pos[0] = value.toFloat(); break;
                case 2: list.at(subRowIndex).mPos.pos[1] = value.toFloat(); break;
                case 3: list.at(subRowIndex).mPos.pos[2] = value.toFloat(); break;
                case 4: list.at(subRowIndex).mPos.rot[0] = value.toFloat(); break;
                case 5: list.at(subRowIndex).mPos.rot[1] = value.toFloat(); break;
                case 6: list.at(subRowIndex).mPos.rot[2] = value.toFloat(); break;
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }

            record.setModified (traveller);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 7;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mTransport.mList.size());
        }
    };

    template <typename ESXRecordT>
    class ActorAiRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        ActorAiRefIdAdapter (const ActorAiRefIdAdapter&);
        ActorAiRefIdAdapter& operator= (const ActorAiRefIdAdapter&);

    public:

        ActorAiRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~ActorAiRefIdAdapter() {}

        // FIXME: should check if the AI package type is already in the list and use a default
        //        that wasn't used already (in extreme case do not add anything at all?
        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT actor = record.get();

            std::vector<ESM::AIPackage>& list = actor.mAiPackage.mList;

            ESM::AIPackage newRow;
            newRow.mType = ESM::AI_Wander;
            newRow.mWander.mDistance = 0;
            newRow.mWander.mDuration = 0;
            newRow.mWander.mTimeOfDay = 0;
            for (int i = 0; i < 8; ++i)
                newRow.mWander.mIdle[i] = 0;
            newRow.mWander.mShouldRepeat = 0;
            newRow.mCellName = "";

            if (position >= (int)list.size())
                list.push_back(newRow);
            else
                list.insert(list.begin()+position, newRow);

            record.setModified (actor);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT actor = record.get();

            std::vector<ESM::AIPackage>& list = actor.mAiPackage.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (actor);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT actor = record.get();

            actor.mAiPackage.mList =
                static_cast<const NestedTableWrapper<std::vector<typename ESM::AIPackage> >&>(nestedTable).mNestedTable;

            record.setModified (actor);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESM::AIPackage> >(record.get().mAiPackage.mList);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<ESM::AIPackage>& list = record.get().mAiPackage.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const ESM::AIPackage& content = list.at(subRowIndex);

            switch (subColIndex)
            {
                case 0:
                    // FIXME: should more than one AI package type be allowed?  Check vanilla
                    switch (content.mType)
                    {
                        case ESM::AI_Wander: return 0;
                        case ESM::AI_Travel: return 1;
                        case ESM::AI_Follow: return 2;
                        case ESM::AI_Escort: return 3;
                        case ESM::AI_Activate: return 4;
                        case ESM::AI_CNDT:
                        default: return QVariant();
                    }
                case 1: // wander dist
                    if (content.mType == ESM::AI_Wander)
                        return content.mWander.mDistance;
                    else
                        return QVariant();
                case 2: // wander dur
                    if (content.mType == ESM::AI_Wander ||
                            content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return content.mWander.mDuration;
                    else
                        return QVariant();
                case 3: // wander ToD
                    if (content.mType == ESM::AI_Wander)
                        return content.mWander.mTimeOfDay; // FIXME: not sure of the format
                    else
                        return QVariant();
                case 4: // wander idle
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    if (content.mType == ESM::AI_Wander)
                        return static_cast<int>(content.mWander.mIdle[subColIndex-4]);
                    else
                        return QVariant();
                case 12: // wander repeat
                    if (content.mType == ESM::AI_Wander)
                        return content.mWander.mShouldRepeat != 0;
                    else
                        return QVariant();
                case 13: // activate name
                    if (content.mType == ESM::AI_Activate)
                        return QString(content.mActivate.mName.toString().c_str());
                    else
                        return QVariant();
                case 14: // target id
                    if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return QString(content.mTarget.mId.toString().c_str());
                    else
                        return QVariant();
                case 15: // target cell
                    if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return QString::fromUtf8(content.mCellName.c_str());
                    else
                        return QVariant();
                case 16:
                    if (content.mType == ESM::AI_Travel)
                        return content.mTravel.mX;
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return content.mTarget.mX;
                    else
                        return QVariant();
                case 17:
                    if (content.mType == ESM::AI_Travel)
                        return content.mTravel.mY;
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return content.mTarget.mY;
                    else
                        return QVariant();
                case 18:
                    if (content.mType == ESM::AI_Travel)
                        return content.mTravel.mZ;
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        return content.mTarget.mZ;
                    else
                        return QVariant();
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT actor = record.get();
            std::vector<ESM::AIPackage>& list = actor.mAiPackage.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            ESM::AIPackage& content = list.at(subRowIndex);

            switch(subColIndex)
            {
                case 0: // ai package type
                    switch (value.toInt())
                    {
                        case 0: content.mType = ESM::AI_Wander; break;
                        case 1: content.mType = ESM::AI_Travel; break;
                        case 2: content.mType = ESM::AI_Follow; break;
                        case 3: content.mType = ESM::AI_Escort; break;
                        case 4: content.mType = ESM::AI_Activate; break;
                        default: return; // return without saving
                    }
                    break; // always save

                case 1:
                    if (content.mType == ESM::AI_Wander)
                        content.mWander.mDistance = static_cast<short>(value.toInt());
                    else
                        return; // return without saving

                    break; // always save
                case 2:
                    if (content.mType == ESM::AI_Wander ||
                            content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mWander.mDuration = static_cast<short>(value.toInt());
                    else
                        return; // return without saving
                case 3:
                    if (content.mType == ESM::AI_Wander)
                        content.mWander.mTimeOfDay = static_cast<unsigned char>(value.toInt());
                    else
                        return; // return without saving

                    break; // always save
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    if (content.mType == ESM::AI_Wander)
                        content.mWander.mIdle[subColIndex-4] = static_cast<unsigned char>(value.toInt());
                    else
                        return; // return without saving

                    break; // always save
                case 12:
                    if (content.mType == ESM::AI_Wander)
                        content.mWander.mShouldRepeat = static_cast<unsigned char>(value.toInt());
                    else
                        return; // return without saving

                    break; // always save
                case 13: // NAME32
                    if (content.mType == ESM::AI_Activate)
                        content.mActivate.mName.assign(value.toString().toUtf8().constData());
                    else
                        return; // return without saving

                    break; // always save
                case 14: // NAME32
                    if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mTarget.mId.assign(value.toString().toUtf8().constData());
                    else
                        return; // return without saving

                    break; // always save
                case 15:
                    if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mCellName = std::string(value.toString().toUtf8().constData());
                    else
                        return; // return without saving

                    break; // always save
                case 16:
                    if (content.mType == ESM::AI_Travel)
                        content.mTravel.mZ = value.toFloat();
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mTarget.mZ = value.toFloat();
                    else
                        return; // return without saving

                    break; // always save
                case 17:
                    if (content.mType == ESM::AI_Travel)
                        content.mTravel.mZ = value.toFloat();
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mTarget.mZ = value.toFloat();
                    else
                        return; // return without saving

                    break; // always save
                case 18:
                    if (content.mType == ESM::AI_Travel)
                        content.mTravel.mZ = value.toFloat();
                    else if (content.mType == ESM::AI_Follow || content.mType == ESM::AI_Escort)
                        content.mTarget.mZ = value.toFloat();
                    else
                        return; // return without saving

                    break; // always save
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }

            record.setModified (actor);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 19;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mAiPackage.mList.size());
        }
    };


    template <typename ESXRecordT>
    class BodyPartRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        BodyPartRefIdAdapter (const BodyPartRefIdAdapter&);
        BodyPartRefIdAdapter& operator= (const BodyPartRefIdAdapter&);

    public:

        BodyPartRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~BodyPartRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT apparel = record.get();

            std::vector<ESM::PartReference>& list = apparel.mParts.mParts;

            ESM::PartReference newPart;
            newPart.mPart = 0; // 0 == head
            newPart.mMale = "";
            newPart.mFemale = "";

            if (position >= (int)list.size())
                list.push_back(newPart);
            else
                list.insert(list.begin()+position, newPart);

            record.setModified (apparel);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT apparel = record.get();

            std::vector<ESM::PartReference>& list = apparel.mParts.mParts;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (apparel);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT apparel = record.get();

            apparel.mParts.mParts =
                static_cast<const NestedTableWrapper<std::vector<typename ESM::PartReference> >&>(nestedTable).mNestedTable;

            record.setModified (apparel);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESM::PartReference> >(record.get().mParts.mParts);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<ESM::PartReference>& list = record.get().mParts.mParts;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const ESM::PartReference& content = list.at(subRowIndex);

            switch (subColIndex)
            {
                case 0:
                {
                    if (content.mPart < ESM::PRT_Count)
                        return content.mPart;
                    else
                        throw std::runtime_error("Part Reference Type unexpected value");
                }
                case 1: return QString(content.mMale.c_str());
                case 2: return QString(content.mFemale.c_str());
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT apparel = record.get();
            std::vector<ESM::PartReference>& list = apparel.mParts.mParts;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            switch(subColIndex)
            {
                case 0: list.at(subRowIndex).mPart = static_cast<unsigned char>(value.toInt()); break;
                case 1: list.at(subRowIndex).mMale = value.toString().toStdString(); break;
                case 2: list.at(subRowIndex).mFemale = value.toString().toStdString(); break;
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }

            record.setModified (apparel);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 3;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mParts.mParts.size());
        }
    };


    struct LevListColumns : public BaseColumns
    {
        const RefIdColumn *mLevList;
        const RefIdColumn *mNestedListLevList;

        LevListColumns (const BaseColumns& base) : BaseColumns (base) {}
    };

    template<typename RecordT>
    class LevelledListRefIdAdapter : public BaseRefIdAdapter<RecordT>
    {
            LevListColumns mLevList;

        public:

            LevelledListRefIdAdapter (UniversalId::Type type, const LevListColumns &columns);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    template<typename RecordT>
    LevelledListRefIdAdapter<RecordT>::LevelledListRefIdAdapter (UniversalId::Type type,
            const LevListColumns &columns)
    : BaseRefIdAdapter<RecordT> (type, columns), mLevList (columns)
    {}

    template<typename RecordT>
    QVariant LevelledListRefIdAdapter<RecordT>::getData (const RefIdColumn *column, const RefIdData& data,
        int index) const
    {
        if (column==mLevList.mLevList || column == mLevList.mNestedListLevList)
            return QVariant::fromValue(ColumnBase::TableEdit_Full);

        return BaseRefIdAdapter<RecordT>::getData (column, data, index);
    }

    template<typename RecordT>
    void LevelledListRefIdAdapter<RecordT>::setData (const RefIdColumn *column, RefIdData& data, int index,
        const QVariant& value) const
    {
        BaseRefIdAdapter<RecordT>::setData (column, data, index, value);
        return;
    }


    // for non-tables
    template <typename ESXRecordT>
    class NestedListLevListRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        NestedListLevListRefIdAdapter (const NestedListLevListRefIdAdapter&);
        NestedListLevListRefIdAdapter& operator= (const NestedListLevListRefIdAdapter&);

    public:

        NestedListLevListRefIdAdapter(UniversalId::Type type)
                :mType(type) {}

        virtual ~NestedListLevListRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            throw std::logic_error ("cannot add a row to a fixed table");
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            throw std::logic_error ("cannot remove a row to a fixed table");
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            throw std::logic_error ("table operation not supported");
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            throw std::logic_error ("table operation not supported");
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            if (mType == UniversalId::Type_CreatureLevelledList)
            {
                switch (subColIndex)
                {
                    case 0: return QVariant(); // disable the checkbox editor
                    case 1: return record.get().mFlags & ESM::CreatureLevList::AllLevels;
                    case 2: return static_cast<int> (record.get().mChanceNone);
                    default:
                        throw std::runtime_error("Trying to access non-existing column in levelled creatues!");
                }
            }
            else
            {
                switch (subColIndex)
                {
                    case 0: return record.get().mFlags & ESM::ItemLevList::Each;
                    case 1: return record.get().mFlags & ESM::ItemLevList::AllLevels;
                    case 2: return static_cast<int> (record.get().mChanceNone);
                    default:
                        throw std::runtime_error("Trying to access non-existing column in levelled items!");
                }
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT leveled = record.get();

            if (mType == UniversalId::Type_CreatureLevelledList)
            {
                switch(subColIndex)
                {
                    case 0: return; // return without saving
                    case 1:
                    {
                        if(value.toBool())
                        {
                            leveled.mFlags |= ESM::CreatureLevList::AllLevels;
                            break;
                        }
                        else
                        {
                            leveled.mFlags &= ~ESM::CreatureLevList::AllLevels;
                            break;
                        }
                    }
                    case 2: leveled.mChanceNone = static_cast<unsigned char>(value.toInt()); break;
                    default:
                        throw std::runtime_error("Trying to set non-existing column in levelled creatures!");
                }
            }
            else
            {
                switch(subColIndex)
                {
                    case 0:
                    {
                        if(value.toBool())
                        {
                            leveled.mFlags |= ESM::ItemLevList::Each;
                            break;
                        }
                        else
                        {
                            leveled.mFlags &= ~ESM::ItemLevList::Each;
                            break;
                        }
                    }
                    case 1:
                    {
                        if(value.toBool())
                        {
                            leveled.mFlags |= ESM::ItemLevList::AllLevels;
                            break;
                        }
                        else
                        {
                            leveled.mFlags &= ~ESM::ItemLevList::AllLevels;
                            break;
                        }
                    }
                    case 2: leveled.mChanceNone = static_cast<unsigned char>(value.toInt()); break;
                    default:
                        throw std::runtime_error("Trying to set non-existing column in levelled items!");
                }
            }

            record.setModified (leveled);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 3;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            return 1; // fixed at size 1
        }
    };

    // for tables
    template <typename ESXRecordT>
    class NestedLevListRefIdAdapter : public NestedRefIdAdapterBase
    {
        UniversalId::Type mType;

        // not implemented
        NestedLevListRefIdAdapter (const NestedLevListRefIdAdapter&);
        NestedLevListRefIdAdapter& operator= (const NestedLevListRefIdAdapter&);

    public:

        NestedLevListRefIdAdapter(UniversalId::Type type) :mType(type) {}

        virtual ~NestedLevListRefIdAdapter() {}

        virtual void addNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int position) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT leveled = record.get();

            std::vector<ESM::LevelledListBase::LevelItem>& list = leveled.mList;

            ESM::LevelledListBase::LevelItem newItem;
            newItem.mId = "";
            newItem.mLevel = 0;

            if (position >= (int)list.size())
                list.push_back(newItem);
            else
                list.insert(list.begin()+position, newItem);

            record.setModified (leveled);
        }

        virtual void removeNestedRow (const RefIdColumn *column,
                RefIdData& data, int index, int rowToRemove) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT leveled = record.get();

            std::vector<ESM::LevelledListBase::LevelItem>& list = leveled.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            list.erase (list.begin () + rowToRemove);

            record.setModified (leveled);
        }

        virtual void setNestedTable (const RefIdColumn* column,
                RefIdData& data, int index, const NestedTableWrapperBase& nestedTable) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));
            ESXRecordT leveled = record.get();

            leveled.mList =
                static_cast<const NestedTableWrapper<std::vector<typename ESM::LevelledListBase::LevelItem> >&>(nestedTable).mNestedTable;

            record.setModified (leveled);
        }

        virtual NestedTableWrapperBase* nestedTable (const RefIdColumn* column,
                const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESM::LevelledListBase::LevelItem> >(record.get().mList);
        }

        virtual QVariant getNestedData (const RefIdColumn *column,
                const RefIdData& data, int index, int subRowIndex, int subColIndex) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            const std::vector<ESM::LevelledListBase::LevelItem>& list = record.get().mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            const ESM::LevelledListBase::LevelItem& content = list.at(subRowIndex);

            switch (subColIndex)
            {
                case 0: return QString(content.mId.c_str());
                case 1: return content.mLevel;
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }
        }

        virtual void setNestedData (const RefIdColumn *column,
                RefIdData& data, int row, const QVariant& value, int subRowIndex, int subColIndex) const
        {
            Record<ESXRecordT>& record =
                static_cast<Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (row, mType)));
            ESXRecordT leveled = record.get();
            std::vector<ESM::LevelledListBase::LevelItem>& list = leveled.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (list.size()))
                throw std::runtime_error ("index out of range");

            switch(subColIndex)
            {
                case 0: list.at(subRowIndex).mId = value.toString().toStdString(); break;
                case 1: list.at(subRowIndex).mLevel = static_cast<short>(value.toInt()); break;
                default:
                    throw std::runtime_error("Trying to access non-existing column in the nested table!");
            }

            record.setModified (leveled);
        }

        virtual int getNestedColumnsCount(const RefIdColumn *column, const RefIdData& data) const
        {
            return 2;
        }

        virtual int getNestedRowsCount(const RefIdColumn *column, const RefIdData& data, int index) const
        {
            const Record<ESXRecordT>& record =
                static_cast<const Record<ESXRecordT>&> (data.getRecord (RefIdData::LocalIndex (index, mType)));

            return static_cast<int>(record.get().mList.size());
        }
    };
}

#endif
