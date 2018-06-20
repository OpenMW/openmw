#include "birthsigncheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadbsgn.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::BirthsignCheckStage::BirthsignCheckStage (const CSMWorld::IdCollection<ESM::BirthSign>& birthsigns)
: mBirthsigns (birthsigns)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::BirthsignCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mBirthsigns.getSize();
}

void CSMTools::BirthsignCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::BirthSign>& record = mBirthsigns.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const ESM::BirthSign& birthsign = record.get();

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Birthsign, birthsign.mId);

    // test for empty name, description and texture
    if (birthsign.mName.empty())
        messages.push_back (std::make_pair (id, birthsign.mId + " has an empty name"));

    if (birthsign.mDescription.empty())
        messages.push_back (std::make_pair (id, birthsign.mId + " has an empty description"));

    if (birthsign.mTexture.empty())
        messages.push_back (std::make_pair (id, birthsign.mId + " is missing a texture"));

    /// \todo test if the texture exists

    /// \todo check data members that can't be edited in the table view
}
