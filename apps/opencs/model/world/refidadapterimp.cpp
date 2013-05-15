
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