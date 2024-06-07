#include "spellcheck.hpp"

#include <string>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm/attr.hpp>
#include <components/esm3/loadspel.hpp>

#include "../prefs/state.hpp"

CSMTools::SpellCheckStage::SpellCheckStage(const CSMWorld::IdCollection<ESM::Spell>& spells)
    : mSpells(spells)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SpellCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSpells.getSize();
}

void CSMTools::SpellCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Spell>& record = mSpells.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Spell& spell = record.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Spell, spell.mId);

    // test for empty name
    if (spell.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    // test for invalid cost values
    if (spell.mData.mCost < 0)
        messages.add(id, "Spell cost is negative", "", CSMDoc::Message::Severity_Error);

    if (spell.mEffects.mList.empty())
    {
        messages.add(id, "Spell doesn't have any magic effects", "", CSMDoc::Message::Severity_Warning);
    }
    else
    {
        std::vector<ESM::IndexedENAMstruct>::const_iterator effect = spell.mEffects.mList.begin();

        for (size_t i = 1; i <= spell.mEffects.mList.size(); i++)
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
