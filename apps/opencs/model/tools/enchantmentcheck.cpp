#include "enchantmentcheck.hpp"

#include <stddef.h>
#include <string>
#include <vector>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>

#include <components/esm/attr.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/loadench.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::EnchantmentCheckStage::EnchantmentCheckStage(const CSMWorld::IdCollection<ESM::Enchantment>& enchantments)
    : mEnchantments(enchantments)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::EnchantmentCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mEnchantments.getSize();
}

void CSMTools::EnchantmentCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Enchantment>& record = mEnchantments.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Enchantment& enchantment = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Enchantment, enchantment.mId);

    if (enchantment.mData.mType < 0 || enchantment.mData.mType > 3)
        messages.add(id, "Invalid type", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mData.mCost < 0)
        messages.add(id, "Cost is negative", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mData.mCharge < 0)
        messages.add(id, "Charge is negative", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mData.mCost > enchantment.mData.mCharge)
        messages.add(id, "Cost is higher than charge", "", CSMDoc::Message::Severity_Error);

    if (enchantment.mEffects.mList.empty())
    {
        messages.add(id, "Enchantment doesn't have any magic effects", "", CSMDoc::Message::Severity_Warning);
    }
    else
    {
        std::vector<ESM::IndexedENAMstruct>::const_iterator effect = enchantment.mEffects.mList.begin();

        for (size_t i = 1; i <= enchantment.mEffects.mList.size(); i++)
        {
            const std::string number = std::to_string(i);
            // At the time of writing this effects, attributes and skills are mostly hardcoded
            if (effect->mData.mEffectID < 0 || effect->mData.mEffectID > ESM::MagicEffect::Length)
                messages.add(id, "Effect #" + number + " is invalid", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mSkill < -1 || effect->mData.mSkill > ESM::Skill::Length)
                messages.add(
                    id, "Effect #" + number + " affected skill is invalid", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mAttribute < -1 || effect->mData.mAttribute > ESM::Attribute::Length)
                messages.add(
                    id, "Effect #" + number + " affected attribute is invalid", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mRange < 0 || effect->mData.mRange > 2)
                messages.add(id, "Effect #" + number + " range is invalid", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mArea < 0)
                messages.add(id, "Effect #" + number + " area is negative", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mDuration < 0)
                messages.add(id, "Effect #" + number + " duration is negative", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mMagnMin < 0)
                messages.add(
                    id, "Effect #" + number + " minimum magnitude is negative", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mMagnMax < 0)
                messages.add(
                    id, "Effect #" + number + " maximum magnitude is negative", "", CSMDoc::Message::Severity_Error);
            if (effect->mData.mMagnMin > effect->mData.mMagnMax)
                messages.add(id, "Effect #" + number + " minimum magnitude is higher than maximum magnitude", "",
                    CSMDoc::Message::Severity_Error);
            ++effect;
        }
    }
}
