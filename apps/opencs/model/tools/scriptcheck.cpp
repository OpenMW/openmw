#include "scriptcheck.hpp"

#include <components/compiler/tokenloc.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>

#include "../doc/document.hpp"

#include "../world/data.hpp"

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

    stream
        << "script " << mFile
        << ", line " << loc.mLine << ", column " << loc.mColumn
        << " (" << loc.mLiteral << "): " << message;

    std::ostringstream hintStream;

    hintStream << "l:" << loc.mLine << " " << loc.mColumn;

    mMessages->add (id, stream.str(), hintStream.str(), getSeverity (type));
}

void CSMTools::ScriptCheckStage::report (const std::string& message, Type type)
{
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

    std::ostringstream stream;
    stream << "script " << mFile << ": " << message;
    
    mMessages->add (id, stream.str(), "", getSeverity (type));
}

CSMTools::ScriptCheckStage::ScriptCheckStage (const CSMDoc::Document& document)
: mDocument (document), mContext (document.getData()), mMessages (0), mWarningMode (Mode_Ignore)
{
    /// \todo add an option to configure warning mode
    setWarningsMode (0);

    Compiler::registerExtensions (mExtensions);
    mContext.setExtensions (&mExtensions);
}

int CSMTools::ScriptCheckStage::setup()
{
    mContext.clear();
    mMessages = 0;
    mId.clear();
    Compiler::ErrorHandler::reset();

    return mDocument.getData().getScripts().getSize();
}

void CSMTools::ScriptCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    mId = mDocument.getData().getScripts().getId (stage);

    if (mDocument.isBlacklisted (
        CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Script, mId)))
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
        const CSMWorld::Data& data = mDocument.getData();

        mFile = data.getScripts().getRecord (stage).get().mId;
        std::istringstream input (data.getScripts().getRecord (stage).get().mScriptText);

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
        stream << "script " << mFile << ": " << error.what();
        
        messages.add (id, stream.str(), "", CSMDoc::Message::Severity_SeriousError);
    }

    mMessages = 0;
}

void CSMTools::ScriptCheckStage::updateUserSetting (const QString& name, const QStringList& value)
{
    if (name=="script-editor/warnings" && !value.isEmpty())
    {
        if (value.at (0)=="Ignore")
            mWarningMode = Mode_Ignore;
        else if (value.at (0)=="Normal")
            mWarningMode = Mode_Normal;
        else if (value.at (0)=="Strict")
            mWarningMode = Mode_Strict;
    }
}
