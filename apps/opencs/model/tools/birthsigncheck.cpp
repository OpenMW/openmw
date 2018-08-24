#include "birthsigncheck.hpp"

#include <sstream>
#include <map>

#include <components/esm/loadbsgn.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

#include "../world/data.hpp"
#include "../world/resources.hpp"
#include "../world/universalid.hpp"

namespace
{
    void addMessage(CSMDoc::Messages &messages, const CSMWorld::UniversalId &id, const std::string& text)
    {
        if (!text.empty())
        {
            messages.push_back(std::make_pair(id, text));
        }
    }
}


std::string CSMTools::BirthsignCheckStage::checkTexture(const std::string &texture) const
{
    if (texture.empty()) return "Texture is missing";
    if (mTextures.searchId(texture) != -1) return std::string();

    std::string ddsTexture = texture;
    if (Misc::ResourceHelpers::changeExtensionToDds(ddsTexture) && mTextures.searchId(ddsTexture) != -1) return std::string();

    return "Texture '" + texture + "' does not exist";
}

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
        addMessage(messages, id, "Name is missing");

    if (birthsign.mDescription.empty())
        addMessage(messages, id, "Description is missing");

    addMessage(messages, id, checkTexture(birthsign.mTexture));

    /// \todo check data members that can't be edited in the table view
}
