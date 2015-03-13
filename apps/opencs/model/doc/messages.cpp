
#include "messages.hpp"

void CSMDoc::Messages::add (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint)
{
    Message data;
    data.mId = id;
    data.mMessage = message;
    data.mHint = hint;

    mMessages.push_back (data);
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
