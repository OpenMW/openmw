
#include "refidadapterimp.hpp"

CSMWorld::PotionRefIdAdapter::PotionRefIdAdapter (const InventoryColumns& columns,
    const RefIdColumn *autoCalc)
: InventoryRefIdAdapter<ESM::Potion> (UniversalId::Type_Potion, columns),
  mAutoCalc (autoCalc)
{}

QVariant CSMWorld::PotionRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Potion>& record = static_cast<const Record<ESM::Potion>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Potion)));

    if (column==mAutoCalc)
        return record.get().mData.mAutoCalc!=0;

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
    const RefIdColumn *type, const RefIdColumn *health, const RefIdColumn *armor)
: EnchantableRefIdAdapter<ESM::Armor> (UniversalId::Type_Armor, columns),
    mType (type), mHealth (health), mArmor (armor)
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
    const RefIdColumn *type)
: EnchantableRefIdAdapter<ESM::Clothing> (UniversalId::Type_Clothing, columns), mType (type)
{}

QVariant CSMWorld::ClothingRefIdAdapter::getData (const RefIdColumn *column,
    const RefIdData& data, int index) const
{
    const Record<ESM::Clothing>& record = static_cast<const Record<ESM::Clothing>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Clothing)));

    if (column==mType)
        return record.get().mData.mType;

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
    const RefIdColumn *weight, const RefIdColumn *organic, const RefIdColumn *respawn)
: NameRefIdAdapter<ESM::Container> (UniversalId::Type_Container, columns), mWeight (weight),
  mOrganic (organic), mRespawn (respawn)
{}

QVariant CSMWorld::ContainerRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
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
: ActorColumns (actorColumns)
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