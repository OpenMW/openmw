#include "scriptcheck.hpp"

#include <components/compiler/tokenloc.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>

#include "../doc/document.hpp"

#include "../world/data.hpp"

#include "../prefs/state.hpp"

CSMDoc::Message::Severity CSMTools::ScriptCheckStage::getSeverity (Type type)
{
    switch (type)
    {
        case WarningMessage: return CSMDoc::Message::Severity_Warning;
        case ErrorMessage: return CSMDoc::Message::Severity_Error;
    }

    return CSMDoc::Message::Severity_SeriousError;
}

void CSMTools::ScriptCheckStage::report (const std::string& message, const Compiler::TokenLoc& loc,
    Type type)
{
    std::ostringstream stream;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

    stream << message << " (" << loc.mLiteral  << ")" << " @ line " << loc.mLine+1 << ", column " << loc.mColumn;

    std::ostringstream hintStream;

    hintStream << "l:" << loc.mLine+1 << " " << loc.mColumn;

    mMessages->add (id, stream.str(), hintStream.str(), getSeverity (type));
}

void CSMTools::ScriptCheckStage::report (const std::string& message, Type type)
{
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

    std::ostringstream stream;
    stream << message;

    mMessages->add (id, stream.str(), "", getSeverity (type));
}

CSMTools::ScriptCheckStage::ScriptCheckStage (const CSMDoc::Document& document)
: mDocument (document), mContext (document.getData()), mMessages (0), mWarningMode (Mode_Ignore)
{
    /// \todo add an option to configure warning mode
    setWarningsMode (0);

    Compiler::registerExtensions (mExtensions);
    mContext.setExtensions (&mExtensions);

    mIgnoreBaseRecords = false;
}

int CSMTools::ScriptCheckStage::setup()
{
    std::string warnings = CSMPrefs::get()["Scripts"]["warnings"].toString();

    if (warnings=="Ignore")
        mWarningMode = Mode_Ignore;
    else if (warnings=="Normal")
        mWarningMode = Mode_Normal;
    else if (warnings=="Strict")
        mWarningMode = Mode_Strict;

    mContext.clear();
    mMessages = 0;
    mId.clear();
    Compiler::ErrorHandler::reset();

    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mDocument.getData().getScripts().getSize();
}

void CSMTools::ScriptCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::Script> &record = mDocument.getData().getScripts().getRecord(stage);

    mId = mDocument.getData().getScripts().getId (stage);

    if (mDocument.isBlacklisted (
        CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Script, mId)))
        return;

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;

    mMessages = &messages;

    switch (mWarningMode)
    {
        case Mode_Ignore: setWarningsMode (0); break;
        case Mode_Normal: setWarningsMode (1); break;
        case Mode_Strict: setWarningsMode (2); break;
    }

    try
    {
        mFile = record.get().mId;
        std::istringstream input (record.get().mScriptText);

        Compiler::Scanner scanner (*this, input, mContext.getExtensions());

        Compiler::FileParser parser (*this, mContext);

        scanner.scan (parser);
    }
    catch (const Compiler::SourceException&)
    {
        // error has already been reported via error handler
    }
    catch (const std::exception& error)
    {
        CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

        std::ostringstream stream;
        stream << error.what();

        messages.add (id, stream.str(), "", CSMDoc::Message::Severity_SeriousError);
    }

    mMessages = 0;
}
