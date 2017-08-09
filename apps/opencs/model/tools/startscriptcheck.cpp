#include "startscriptcheck.hpp"

#include <components/misc/stringops.hpp>

CSMTools::StartScriptCheckStage::StartScriptCheckStage (
    const CSMWorld::IdCollection<ESM::StartScript>& startScripts,
    const CSMWorld::IdCollection<ESM::Script>& scripts)
: mStartScripts (startScripts), mScripts (scripts)
{}

void CSMTools::StartScriptCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::StartScript>& record = mStartScripts.getRecord (stage);

    if (record.isDeleted())
        return;

    std::string scriptId = record.get().mId;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_StartScript, scriptId);

    if (mScripts.searchId (Misc::StringUtils::lowerCase (scriptId))==-1)
        messages.push_back (
            std::make_pair (id, "Start script " + scriptId + " does not exist"));
}

int CSMTools::StartScriptCheckStage::setup()
{
    return mStartScripts.getSize();
}
