#include "spellcheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadspel.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::SpellCheckStage::SpellCheckStage (const CSMWorld::IdCollection<ESM::Spell>& spells)
: mSpells (spells)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::SpellCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mSpells.getSize();
}

void CSMTools::SpellCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Spell>& record = mSpells.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::Spell& spell = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Spell, spell.mId);

    // test for empty name and description
    if (spell.mName.empty())
        messages.push_back (std::make_pair (id, spell.mId + " has an empty name"));

    // test for invalid cost values
    if (spell.mData.mCost<0)
        messages.push_back (std::make_pair (id, spell.mId + " has a negative spell costs"));

    /// \todo check data members that can't be edited in the table view
}
