#include "messages.hpp"

CSMDoc::Message::Message() : mSeverity(Severity_Default){}

CSMDoc::Message::Message (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint, Severity severity)
: mId (id), mMessage (message), mHint (hint), mSeverity (severity)
{}

std::string CSMDoc::Message::toString (Severity severity)
{
    switch (severity)
    {
        case CSMDoc::Message::Severity_Info: return "Information";
        case CSMDoc::Message::Severity_Warning: return "Warning";
        case CSMDoc::Message::Severity_Error: return "Error";
        case CSMDoc::Message::Severity_SeriousError: return "Serious Error";
        case CSMDoc::Message::Severity_Default: break;
    }

    return "";
}


CSMDoc::Messages::Messages (Message::Severity default_)
: mDefault (default_)
{}

void CSMDoc::Messages::add (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint, Message::Severity severity)
{
    if (severity==Message::Severity_Default)
        severity = mDefault;

    mMessages.push_back (Message (id, message, hint, severity));
}

CSMDoc::Messages::Iterator CSMDoc::Messages::begin() const
{
    return mMessages.begin();
}

CSMDoc::Messages::Iterator CSMDoc::Messages::end() const
{
    return mMessages.end();
}
