
#include "spellcheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadspel.hpp>

#include "../world/universalid.hpp"

CSMTools::SpellCheckStage::SpellCheckStage (const CSMWorld::IdCollection<ESM::Spell>& spells)
: mSpells (spells)
{}

int CSMTools::SpellCheckStage::setup()
{
    return mSpells.getSize();
}

void CSMTools::SpellCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    const ESM::Spell& spell = mSpells.getRecord (stage).get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Spell, spell.mId);

    // test for empty name and description
    if (spell.mName.empty())
        messages.push_back (id.toString() + "|" + spell.mId + " has an empty name");

    // test for invalid cost values
    if (spell.mData.mCost<0)
        messages.push_back (id.toString() + "|" + spell.mId + " has a negative spell costs");

    /// \todo check data members that can't be edited in the table view
}