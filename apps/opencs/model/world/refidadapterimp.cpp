
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
        return record.get().mData.mAutoCalc;

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