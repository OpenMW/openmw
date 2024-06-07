#include "spellcheck.hpp"

#include <string>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadspel.hpp>

#include "../prefs/state.hpp"

#include "effectlistcheck.hpp"

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

    effectListCheck(spell.mEffects.mList, messages, id);
}
