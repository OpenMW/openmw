#include "topic.hpp"

#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWDialogue
{
    Topic::Topic() {}

    Topic::Topic(const ESM::RefId& topic)
        : mTopic(topic)
        , mName(MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>().find(topic)->mStringId)
    {
    }

    bool Topic::addEntry(const JournalEntry& entry)
    {
        if (entry.mTopic != mTopic)
            throw std::runtime_error("topic does not match: " + mTopic.toDebugString());

        // bail out if we already have heard this
        for (Topic::TEntryIter it = mEntries.begin(); it != mEntries.end(); ++it)
        {
            if (it->mInfoId == entry.mInfoId)
                return false;
        }

        mEntries.push_back(entry); // we want slicing here
        return false;
    }

    void Topic::insertEntry(const ESM::JournalEntry& entry)
    {
        mEntries.push_back(entry);
    }

    const ESM::RefId& Topic::getTopic() const
    {
        return mTopic;
    }

    std::string_view Topic::getName() const
    {
        return mName;
    }

    Topic::TEntryIter Topic::begin() const
    {
        return mEntries.begin();
    }

    Topic::TEntryIter Topic::end() const
    {
        return mEntries.end();
    }

    void Topic::removeLastAddedResponse(std::string_view actorName)
    {
        for (std::vector<MWDialogue::Entry>::reverse_iterator it = mEntries.rbegin(); it != mEntries.rend(); ++it)
        {
            if (it->mActorName == actorName)
            {
                mEntries.erase((++it).base()); // erase doesn't take a reverse_iterator
                return;
            }
        }
    }
}
