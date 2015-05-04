#include "refidadapterimp.hpp"

#include <cassert>
#include <stdexcept>
#include <utility>

#include <components/esm/loadcont.hpp>
#include "nestedtablewrapper.hpp"

CSMWorld::PotionColumns::PotionColumns (const InventoryColumns& columns)
: InventoryColumns (columns) {}

CSMWorld::PotionRefIdAdapter::PotionRefIdAdapter (const PotionColumns& columns,
    const RefIdColumn *autoCalc)
: InventoryRefIdAdapter<ESM::Potion> (UniversalId::Type_Potion, columns),
  mAutoCalc (autoCalc), mColumns(columns)
{}

QVariant CSMWorld::PotionRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Potion>& record = static_cast<const Record<ESM::Potion>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Potion)));

    if (column==mAutoCalc)
        return record.get().mData.mAutoCalc!=0;

    if (column==mColumns.mEffects)
        return true; // to show nested tables in dialogue subview, see IdTree::hasChildren()

    return InventoryRefIdAdapter<ESM::Potion>::getData (column, data, index);
}

void CSMWorld::PotionRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Potion>& record = static_cast<Record<ESM::Potion>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Potion)));

    if (column==mAutoCalc)
        record.get().mData.mAutoCalc = value.toInt();
    else
        InventoryRefIdAdapter<ESM::Potion>::setData (column, data, index, value);
}


CSMWorld::ApparatusRefIdAdapter::ApparatusRefIdAdapter (const InventoryColumns& columns,
    const RefIdColumn *type, const RefIdColumn *quality)
: InventoryRefIdAdapter<ESM::Apparatus> (UniversalId::Type_Apparatus, columns),
    mType (type), mQuality (quality)
{}

QVariant CSMWorld::ApparatusRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Apparatus>& record = static_cast<const Record<ESM::Apparatus>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Apparatus)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mQuality)
        return record.get().mData.mQuality;

    return InventoryRefIdAdapter<ESM::Apparatus>::getData (column, data, index);
}

void CSMWorld::ApparatusRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Apparatus>& record = static_cast<Record<ESM::Apparatus>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Apparatus)));

    if (column==mType)
        record.get().mData.mType = value.toInt();
    else if (column==mQuality)
        record.get().mData.mQuality = value.toFloat();
    else
        InventoryRefIdAdapter<ESM::Apparatus>::setData (column, data, index, value);
}


CSMWorld::ArmorRefIdAdapter::ArmorRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *type, const RefIdColumn *health, const RefIdColumn *armor,
    const RefIdColumn *partRef)
: EnchantableRefIdAdapter<ESM::Armor> (UniversalId::Type_Armor, columns),
    mType (type), mHealth (health), mArmor (armor), mPartRef(partRef)
{}

QVariant CSMWorld::ArmorRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Armor>& record = static_cast<const Record<ESM::Armor>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Armor)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mHealth)
        return record.get().mData.mHealth;

    if (column==mArmor)
        return record.get().mData.mArmor;

    if (column==mPartRef)
        return true; // to show nested tables in dialogue subview, see IdTree::hasChildren()

    return EnchantableRefIdAdapter<ESM::Armor>::getData (column, data, index);
}

void CSMWorld::ArmorRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Armor>& record = static_cast<Record<ESM::Armor>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Armor)));

    if (column==mType)
        record.get().mData.mType = value.toInt();
    else if (column==mHealth)
        record.get().mData.mHealth = value.toInt();
    else if (column==mArmor)
        record.get().mData.mArmor = value.toInt();
    else
        EnchantableRefIdAdapter<ESM::Armor>::setData (column, data, index, value);
}

CSMWorld::BookRefIdAdapter::BookRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *scroll, const RefIdColumn *skill)
: EnchantableRefIdAdapter<ESM::Book> (UniversalId::Type_Book, columns),
    mScroll (scroll), mSkill (skill)
{}

QVariant CSMWorld::BookRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Book>& record = static_cast<const Record<ESM::Book>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Book)));

    if (column==mScroll)
        return record.get().mData.mIsScroll!=0;

    if (column==mSkill)
        return record.get().mData.mSkillID;

    return EnchantableRefIdAdapter<ESM::Book>::getData (column, data, index);
}

void CSMWorld::BookRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Book>& record = static_cast<Record<ESM::Book>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Book)));

    if (column==mScroll)
        record.get().mData.mIsScroll = value.toInt();
    else if (column==mSkill)
        record.get().mData.mSkillID = value.toInt();
    else
        EnchantableRefIdAdapter<ESM::Book>::setData (column, data, index, value);
}

CSMWorld::ClothingRefIdAdapter::ClothingRefIdAdapter (const EnchantableColumns& columns,
    const RefIdColumn *type, const RefIdColumn *partRef)
: EnchantableRefIdAdapter<ESM::Clothing> (UniversalId::Type_Clothing, columns), mType (type),
  mPartRef(partRef)
{}

QVariant CSMWorld::ClothingRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Clothing>& record = static_cast<const Record<ESM::Clothing>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Clothing)));

    if (column==mType)
        return record.get().mData.mType;

    if (column==mPartRef)
        return true; // to show nested tables in dialogue subview, see IdTree::hasChildren()

    return EnchantableRefIdAdapter<ESM::Clothing>::getData (column, data, index);
}

void CSMWorld::ClothingRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Clothing>& record = static_cast<Record<ESM::Clothing>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Clothing)));

    if (column==mType)
        record.get().mData.mType = value.toInt();
    else
        EnchantableRefIdAdapter<ESM::Clothing>::setData (column, data, index, value);
}

CSMWorld::ContainerRefIdAdapter::ContainerRefIdAdapter (const NameColumns& columns,
    const RefIdColumn *weight, const RefIdColumn *organic, const RefIdColumn *respawn, const RefIdColumn *content)
: NameRefIdAdapter<ESM::Container> (UniversalId::Type_Container, columns), mWeight (weight),
  mOrganic (organic), mRespawn (respawn), mContent(content)
{}

QVariant CSMWorld::ContainerRefIdAdapter::getData (const RefIdColumn *column,
                                                   const RefIdData& data,
                                                   int index) const
{
    const Record<ESM::Container>& record = static_cast<const Record<ESM::Container>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Container)));

    if (column==mWeight)
        return record.get().mWeight;

    if (column==mOrganic)
        return (record.get().mFlags & ESM::Container::Organic)!=0;

    if (column==mRespawn)
        return (record.get().mFlags & ESM::Container::Respawn)!=0;

    if (column==mContent)
        return true; // Required to show nested tables in dialogue subview

    return NameRefIdAdapter<ESM::Container>::getData (column, data, index);
}

void CSMWorld::ContainerRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Container>& record = static_cast<Record<ESM::Container>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Container)));

    if (column==mWeight)
        record.get().mWeight = value.toFloat();
    else if (column==mOrganic)
    {
        if (value.toInt())
            record.get().mFlags |= ESM::Container::Organic;
        else
            record.get().mFlags &= ~ESM::Container::Organic;
    }
    else if (column==mRespawn)
    {
        if (value.toInt())
            record.get().mFlags |= ESM::Container::Respawn;
        else
            record.get().mFlags &= ~ESM::Container::Respawn;
    }
    else
        NameRefIdAdapter<ESM::Container>::setData (column, data, index, value);
}

CSMWorld::CreatureColumns::CreatureColumns (const ActorColumns& actorColumns)
: ActorColumns (actorColumns),
  mType(NULL),
  mSoul(NULL),
  mScale(NULL),
  mOriginal(NULL),
  mCombat(NULL),
  mMagic(NULL),
  mStealth(NULL)
{}

CSMWorld::CreatureRefIdAdapter::CreatureRefIdAdapter (const CreatureColumns& columns)
: ActorRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature, columns), mColumns (columns)
{}

QVariant CSMWorld::CreatureRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Creature>& record = static_cast<const Record<ESM::Creature>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    if (column==mColumns.mType)
        return record.get().mData.mType;

    if (column==mColumns.mSoul)
        return record.get().mData.mSoul;

    if (column==mColumns.mScale)
        return record.get().mScale;

    if (column==mColumns.mOriginal)
        return QString::fromUtf8 (record.get().mOriginal.c_str());

    if (column==mColumns.mCombat)
        return static_cast<int> (record.get().mData.mCombat);

    if (column==mColumns.mMagic)
        return static_cast<int> (record.get().mData.mMagic);

    if (column==mColumns.mStealth)
        return static_cast<int> (record.get().mData.mStealth);

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mFlags & iter->second)!=0;

    return ActorRefIdAdapter<ESM::Creature>::getData (column, data, index);
}

void CSMWorld::CreatureRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Creature>& record = static_cast<Record<ESM::Creature>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Creature)));

    if (column==mColumns.mType)
        record.get().mData.mType = value.toInt();
    else if (column==mColumns.mSoul)
        record.get().mData.mSoul = value.toInt();
    else if (column==mColumns.mScale)
        record.get().mScale = value.toFloat();
    else if (column==mColumns.mOriginal)
        record.get().mOriginal = value.toString().toUtf8().constData();
    else if (column==mColumns.mCombat)
        record.get().mData.mCombat = value.toInt();
    else if (column==mColumns.mMagic)
        record.get().mData.mMagic = value.toInt();
    else if (column==mColumns.mStealth)
        record.get().mData.mStealth = value.toInt();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                record.get().mFlags |= iter->second;
            else
                record.get().mFlags &= ~iter->second;
        }
        else
            ActorRefIdAdapter<ESM::Creature>::setData (column, data, index, value);
    }
}

CSMWorld::DoorRefIdAdapter::DoorRefIdAdapter (const NameColumns& columns,
    const RefIdColumn *openSound, const RefIdColumn *closeSound)
: NameRefIdAdapter<ESM::Door> (UniversalId::Type_Door, columns), mOpenSound (openSound),
  mCloseSound (closeSound)
{}

QVariant CSMWorld::DoorRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Door>& record = static_cast<const Record<ESM::Door>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Door)));

    if (column==mOpenSound)
        return QString::fromUtf8 (record.get().mOpenSound.c_str());

    if (column==mCloseSound)
        return QString::fromUtf8 (record.get().mCloseSound.c_str());

    return NameRefIdAdapter<ESM::Door>::getData (column, data, index);
}

void CSMWorld::DoorRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Door>& record = static_cast<Record<ESM::Door>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Door)));

    if (column==mOpenSound)
        record.get().mOpenSound = value.toString().toUtf8().constData();
    else if (column==mCloseSound)
        record.get().mCloseSound = value.toString().toUtf8().constData();
    else
        NameRefIdAdapter<ESM::Door>::setData (column, data, index, value);
}

CSMWorld::LightColumns::LightColumns (const InventoryColumns& columns)
: InventoryColumns (columns) {}

CSMWorld::LightRefIdAdapter::LightRefIdAdapter (const LightColumns& columns)
: InventoryRefIdAdapter<ESM::Light> (UniversalId::Type_Light, columns), mColumns (columns)
{}

QVariant CSMWorld::LightRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Light>& record = static_cast<const Record<ESM::Light>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Light)));

    if (column==mColumns.mTime)
        return record.get().mData.mTime;

    if (column==mColumns.mRadius)
        return record.get().mData.mRadius;

    if (column==mColumns.mColor)
        return record.get().mData.mColor;

    if (column==mColumns.mSound)
        return QString::fromUtf8 (record.get().mSound.c_str());

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mData.mFlags & iter->second)!=0;

    return InventoryRefIdAdapter<ESM::Light>::getData (column, data, index);
}

void CSMWorld::LightRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Light>& record = static_cast<Record<ESM::Light>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Light)));

    if (column==mColumns.mTime)
        record.get().mData.mTime = value.toInt();
    else if (column==mColumns.mRadius)
        record.get().mData.mRadius = value.toInt();
    else if (column==mColumns.mColor)
        record.get().mData.mColor = value.toInt();
    else if (column==mColumns.mSound)
        record.get().mSound = value.toString().toUtf8().constData();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                record.get().mData.mFlags |= iter->second;
            else
                record.get().mData.mFlags &= ~iter->second;
        }
        else
            InventoryRefIdAdapter<ESM::Light>::setData (column, data, index, value);
    }
}

CSMWorld::MiscRefIdAdapter::MiscRefIdAdapter (const InventoryColumns& columns, const RefIdColumn *key)
: InventoryRefIdAdapter<ESM::Miscellaneous> (UniversalId::Type_Miscellaneous, columns), mKey (key)
{}

QVariant CSMWorld::MiscRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Miscellaneous>& record = static_cast<const Record<ESM::Miscellaneous>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Miscellaneous)));

    if (column==mKey)
        return record.get().mData.mIsKey!=0;

    return InventoryRefIdAdapter<ESM::Miscellaneous>::getData (column, data, index);
}

void CSMWorld::MiscRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Miscellaneous>& record = static_cast<Record<ESM::Miscellaneous>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Miscellaneous)));

    if (column==mKey)
        record.get().mData.mIsKey = value.toInt();
    else
        InventoryRefIdAdapter<ESM::Miscellaneous>::setData (column, data, index, value);
}

CSMWorld::NpcColumns::NpcColumns (const ActorColumns& actorColumns)
: ActorColumns (actorColumns),
  mRace(NULL),
  mClass(NULL),
  mFaction(NULL),
  mHair(NULL),
  mHead(NULL)
{}

CSMWorld::NpcRefIdAdapter::NpcRefIdAdapter (const NpcColumns& columns)
: ActorRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc, columns), mColumns (columns)
{}

QVariant CSMWorld::NpcRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data, int index)
    const
{
    const Record<ESM::NPC>& record = static_cast<const Record<ESM::NPC>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    if (column==mColumns.mRace)
        return QString::fromUtf8 (record.get().mRace.c_str());

    if (column==mColumns.mClass)
        return QString::fromUtf8 (record.get().mClass.c_str());

    if (column==mColumns.mFaction)
        return QString::fromUtf8 (record.get().mFaction.c_str());

    if (column==mColumns.mHair)
        return QString::fromUtf8 (record.get().mHair.c_str());

    if (column==mColumns.mHead)
        return QString::fromUtf8 (record.get().mHead.c_str());

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mFlags & iter->second)!=0;

    return ActorRefIdAdapter<ESM::NPC>::getData (column, data, index);
}

void CSMWorld::NpcRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::NPC>& record = static_cast<Record<ESM::NPC>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Npc)));

    if (column==mColumns.mRace)
        record.get().mRace = value.toString().toUtf8().constData();
    else if (column==mColumns.mClass)
        record.get().mClass = value.toString().toUtf8().constData();
    else if (column==mColumns.mFaction)
        record.get().mFaction = value.toString().toUtf8().constData();
    else if (column==mColumns.mHair)
        record.get().mHair = value.toString().toUtf8().constData();
    else if (column==mColumns.mHead)
        record.get().mHead = value.toString().toUtf8().constData();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                record.get().mFlags |= iter->second;
            else
                record.get().mFlags &= ~iter->second;
        }
        else
            ActorRefIdAdapter<ESM::NPC>::setData (column, data, index, value);
    }
}

CSMWorld::WeaponColumns::WeaponColumns (const EnchantableColumns& columns)
: EnchantableColumns (columns) {}

CSMWorld::WeaponRefIdAdapter::WeaponRefIdAdapter (const WeaponColumns& columns)
: EnchantableRefIdAdapter<ESM::Weapon> (UniversalId::Type_Weapon, columns), mColumns (columns)
{}

QVariant CSMWorld::WeaponRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Weapon>& record = static_cast<const Record<ESM::Weapon>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Weapon)));

    if (column==mColumns.mType)
        return record.get().mData.mType;

    if (column==mColumns.mHealth)
        return record.get().mData.mHealth;

    if (column==mColumns.mSpeed)
        return record.get().mData.mSpeed;

    if (column==mColumns.mReach)
        return record.get().mData.mReach;

    for (int i=0; i<2; ++i)
    {
        if (column==mColumns.mChop[i])
            return record.get().mData.mChop[i];

        if (column==mColumns.mSlash[i])
            return record.get().mData.mSlash[i];

        if (column==mColumns.mThrust[i])
            return record.get().mData.mThrust[i];
    }

    std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
        mColumns.mFlags.find (column);

    if (iter!=mColumns.mFlags.end())
        return (record.get().mData.mFlags & iter->second)!=0;

    return EnchantableRefIdAdapter<ESM::Weapon>::getData (column, data, index);
}

void CSMWorld::WeaponRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Weapon>& record = static_cast<Record<ESM::Weapon>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Weapon)));

    if (column==mColumns.mType)
        record.get().mData.mType = value.toInt();
    else if (column==mColumns.mHealth)
        record.get().mData.mHealth = value.toInt();
    else if (column==mColumns.mSpeed)
        record.get().mData.mSpeed = value.toFloat();
    else if (column==mColumns.mReach)
        record.get().mData.mReach = value.toFloat();
    else if (column==mColumns.mChop[0])
        record.get().mData.mChop[0] = value.toInt();
    else if (column==mColumns.mChop[1])
        record.get().mData.mChop[1] = value.toInt();
    else if (column==mColumns.mSlash[0])
        record.get().mData.mSlash[0] = value.toInt();
    else if (column==mColumns.mSlash[1])
        record.get().mData.mSlash[1] = value.toInt();
    else if (column==mColumns.mThrust[0])
        record.get().mData.mThrust[0] = value.toInt();
    else if (column==mColumns.mThrust[1])
        record.get().mData.mThrust[1] = value.toInt();
    else
    {
        std::map<const RefIdColumn *, unsigned int>::const_iterator iter =
            mColumns.mFlags.find (column);

        if (iter!=mColumns.mFlags.end())
        {
            if (value.toInt()!=0)
                record.get().mData.mFlags |= iter->second;
            else
                record.get().mData.mFlags &= ~iter->second;
        }
        else
            EnchantableRefIdAdapter<ESM::Weapon>::setData (column, data, index, value);
    }
}
