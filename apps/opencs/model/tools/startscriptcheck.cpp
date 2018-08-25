#include "startscriptcheck.hpp"

#include "../prefs/state.hpp"

#include <components/misc/stringops.hpp>

CSMTools::StartScriptCheckStage::StartScriptCheckStage (
    const CSMWorld::IdCollection<ESM::StartScript>& startScripts,
    const CSMWorld::IdCollection<ESM::Script>& scripts)
: mStartScripts (startScripts), mScripts (scripts)
{
    mIgnoreBaseRecords = false;
}

void CSMTools::StartScriptCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::StartScript>& record = mStartScripts.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    std::string scriptId = record.get().mId;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_StartScript, scriptId);

    if (mScripts.searchId (Misc::StringUtils::lowerCase (scriptId))==-1)
        messages.add(id, "Start script " + scriptId + " does not exist", "", CSMDoc::Message::Severity_Error);
}

int CSMTools::StartScriptCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mStartScripts.getSize();
}
