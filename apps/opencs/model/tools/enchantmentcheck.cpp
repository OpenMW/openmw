#include "enchantmentcheck.hpp"

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::EnchantmentCheckStage::EnchantmentCheckStage (const CSMWorld::IdCollection<ESM::Enchantment>& enchantments)
    : mEnchantments (enchantments)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::EnchantmentCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mEnchantments.getSize();
}

void CSMTools::EnchantmentCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Enchantment>& record = mEnchantments.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Enchantment& enchantment = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Enchantment, enchantment.mId);

    if (enchantment.mData.mType < 0 || enchantment.mData.mType > 3)
        messages.add(id, "Invalid type", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mData.mCost < 0)
        messages.add(id, "Cost is negative", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mData.mCharge < 0)
        messages.add(id, "Charge is negative", "", CSMDoc::Message::Severity_Error);

    /// \todo Check effects list for validity
}
