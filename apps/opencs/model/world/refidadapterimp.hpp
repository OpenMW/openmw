#ifndef CSM_WOLRD_REFIDADAPTERIMP_H
#define CSM_WOLRD_REFIDADAPTERIMP_H

#include <map>

#include <QVariant>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>

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

        if (column==mInventory.mIcon)
            record.get().mIcon = value.toString().toUtf8().constData();
        else if (column==mInventory.mWeight)
            record.get().mData.mWeight = value.toFloat();
        else if (column==mInventory.mValue)
            record.get().mData.mValue = value.toInt();
        else
            NameRefIdAdapter<RecordT>::setData (column, data, index, value);
    }

    class PotionRefIdAdapter : public InventoryRefIdAdapter<ESM::Potion>
    {
            const RefIdColumn *mAutoCalc;

        public:

            PotionRefIdAdapter (const InventoryColumns& columns, const RefIdColumn *autoCalc);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
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

        if (column==mEnchantable.mEnchantment)
            record.get().mEnchant = value.toString().toUtf8().constData();
        else if (column==mEnchantable.mEnchantmentPoints)
            record.get().mData.mEnchant = value.toInt();
        else
            InventoryRefIdAdapter<RecordT>::setData (column, data, index, value);
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

        if (column==mTools.mQuality)
            record.get().mData.mQuality = value.toFloat();
        else if (column==mTools.mUses)
            record.get().mData.mUses = value.toInt();
        else
            InventoryRefIdAdapter<RecordT>::setData (column, data, index, value);
    }

    struct ActorColumns : public NameColumns
    {
        const RefIdColumn *mHasAi;
        const RefIdColumn *mHello;
        const RefIdColumn *mFlee;
        const RefIdColumn *mFight;
        const RefIdColumn *mAlarm;
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

        if (column==mActors.mHasAi)
            return record.get().mHasAI!=0;

        if (column==mActors.mHello)
            return record.get().mAiData.mHello;

        if (column==mActors.mFlee)
            return record.get().mAiData.mFlee;

        if (column==mActors.mFight)
            return record.get().mAiData.mFight;

        if (column==mActors.mAlarm)
            return record.get().mAiData.mAlarm;

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

        if (column==mActors.mHasAi)
            record.get().mHasAI = value.toInt();
        else if (column==mActors.mHello)
            record.get().mAiData.mHello = value.toInt();
        else if (column==mActors.mFlee)
            record.get().mAiData.mFlee = value.toInt();
        else if (column==mActors.mFight)
            record.get().mAiData.mFight = value.toInt();
        else if (column==mActors.mAlarm)
            record.get().mAiData.mAlarm = value.toInt();
        else
        {
            typename std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
                mActors.mServices.find (column);
            if (iter!=mActors.mServices.end())
            {
                if (value.toInt()!=0)
                    record.get().mAiData.mServices |= iter->second;
                else
                    record.get().mAiData.mServices &= ~iter->second;
            }
            else
                NameRefIdAdapter<RecordT>::setData (column, data, index, value);
        }
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

        public:

            ArmorRefIdAdapter (const EnchantableColumns& columns, const RefIdColumn *type,
                const RefIdColumn *health, const RefIdColumn *armor);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class BookRefIdAdapter : public EnchantableRefIdAdapter<ESM::Book>
    {
            const RefIdColumn *mScroll;
            const RefIdColumn *mSkill;

        public:

            BookRefIdAdapter (const EnchantableColumns& columns, const RefIdColumn *scroll,
                const RefIdColumn *skill);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    class ClothingRefIdAdapter : public EnchantableRefIdAdapter<ESM::Clothing>
    {
            const RefIdColumn *mType;

        public:

            ClothingRefIdAdapter (const EnchantableColumns& columns, const RefIdColumn *type);

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

        public:

            ContainerRefIdAdapter (const NameColumns& columns, const RefIdColumn *weight,
                const RefIdColumn *organic, const RefIdColumn *respawn);

            virtual QVariant getData (const RefIdColumn *column, const RefIdData& data, int index)
                const;

            virtual void setData (const RefIdColumn *column, RefIdData& data, int index,
                const QVariant& value) const;
            ///< If the data type does not match an exception is thrown.
    };

    struct CreatureColumns : public ActorColumns
    {
        std::map<const RefIdColumn *, unsigned int> mFlags;
        const RefIdColumn *mType;
        const RefIdColumn *mSoul;
        const RefIdColumn *mScale;
        const RefIdColumn *mOriginal;
        const RefIdColumn *mCombat;
        const RefIdColumn *mMagic;
        const RefIdColumn *mStealth;

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
}

#endif
