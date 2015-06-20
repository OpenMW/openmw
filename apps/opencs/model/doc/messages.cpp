
#include "messages.hpp"

CSMDoc::Message::Message() {}

CSMDoc::Message::Message (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint, Severity severity)
: mId (id), mMessage (message), mHint (hint), mSeverity (severity)
{}


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

void CSMDoc::Messages::push_back (const std::pair<CSMWorld::UniversalId, std::string>& data)
{
    add (data.first, data.second);
}

CSMDoc::Messages::Iterator CSMDoc::Messages::begin() const
{
    return mMessages.begin();
}

CSMDoc::Messages::Iterator CSMDoc::Messages::end() const
{
    return mMessages.end();
}
