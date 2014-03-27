
#include "scriptcheck.hpp"

#include <components/compiler/tokenloc.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/extensions0.hpp>

#include "../world/data.hpp"

void CSMTools::ScriptCheckStage::report (const std::string& message, const Compiler::TokenLoc& loc,
    Type type)
{
    std::ostringstream stream;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

    stream << id.toString() << "|";

    if (type==ErrorMessage)
        stream << "error ";
    else
        stream << "warning ";

    stream
        << "script " << mFile
        << ", line " << loc.mLine << ", column " << loc.mColumn
        << " (" << loc.mLiteral << "): " << message;

    mMessages->push_back (stream.str());
}

void CSMTools::ScriptCheckStage::report (const std::string& message, Type type)
{
    std::ostringstream stream;

    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

    stream << id.toString() << "|";

    if (type==ErrorMessage)
        stream << "error: ";
    else
        stream << "warning: ";

    stream << message;

    mMessages->push_back (stream.str());
}

CSMTools::ScriptCheckStage::ScriptCheckStage (const CSMWorld::Data& data)
: mData (data), mContext (data), mMessages (0)
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

    return mData.getScripts().getSize();
}

void CSMTools::ScriptCheckStage::perform (int stage, std::vector<std::string>& messages)
{
    mMessages = &messages;
    mId = mData.getScripts().getId (stage);

    try
    {
        mFile = mData.getScripts().getRecord (stage).get().mId;
        std::istringstream input (mData.getScripts().getRecord (stage).get().mScriptText);

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
        std::ostringstream stream;

        CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Script, mId);

        stream << id.toString() << "|Critical compile error: " << error.what();

        messages.push_back (stream.str());
    }

    mMessages = 0;
}