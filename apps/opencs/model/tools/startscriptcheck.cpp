#include "startscriptcheck.hpp"

#include <string>

#include "../prefs/state.hpp"

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadsscr.hpp>
#include <components/misc/strings/lower.hpp>

namespace ESM
{
    class Script;
}

CSMTools::StartScriptCheckStage::StartScriptCheckStage(
    const CSMWorld::IdCollection<ESM::StartScript>& startScripts, const CSMWorld::IdCollection<ESM::Script>& scripts)
    : mStartScripts(startScripts)
    , mScripts(scripts)
{
    mIgnoreBaseRecords = false;
}

void CSMTools::StartScriptCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::StartScript>& record = mStartScripts.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    const auto& scriptId = record.get().mId;

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_StartScript, scriptId);

    if (mScripts.searchId(scriptId) == -1)
        messages.add(
            id, "Start script " + scriptId.getRefIdString() + " does not exist", "", CSMDoc::Message::Severity_Error);
}

int CSMTools::StartScriptCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mStartScripts.getSize();
}
