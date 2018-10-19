#include "birthsigncheck.hpp"

#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

#include "../world/universalid.hpp"

CSMTools::BirthsignCheckStage::BirthsignCheckStage (const CSMWorld::IdCollection<ESM::BirthSign>& birthsigns,
                                                    const CSMWorld::Resources &textures)
: mBirthsigns(birthsigns),
  mTextures(textures)
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

    if (birthsign.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    if (birthsign.mDescription.empty())
        messages.add(id, "Description is missing", "", CSMDoc::Message::Severity_Warning);

    if (birthsign.mTexture.empty())
        messages.add(id, "Image is missing", "", CSMDoc::Message::Severity_Error);
    else if (mTextures.searchId(birthsign.mTexture) == -1)
    {
        std::string ddsTexture = birthsign.mTexture;
        if (!(Misc::ResourceHelpers::changeExtensionToDds(ddsTexture) && mTextures.searchId(ddsTexture) != -1))
            messages.add(id,  "Image '" + birthsign.mTexture + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }

    /// \todo check data members that can't be edited in the table view
}
