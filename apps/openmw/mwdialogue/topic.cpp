
#include "topic.hpp"

#include <components/esm_store/store.hpp>

namespace MWDialogue
{
    Topic::Topic()
    {}

    Topic::Topic (const std::string& topic)
    : mTopic (topic)
    {}

    Topic::~Topic()
    {}

    void Topic::addEntry (const JournalEntry& entry)
    {
        if (entry.mTopic!=mTopic)
            throw std::runtime_error ("topic does not match: " + mTopic);

        for (TEntryIter iter = begin(); iter!=end(); ++iter)
            if (*iter==entry.mInfoId)
                return;

        mEntries.push_back (entry.mInfoId);
    }

    Topic::TEntryIter Topic::begin()
    {
        return mEntries.begin();
    }

    Topic::TEntryIter Topic::end()
    {
        return mEntries.end();
    }

    JournalEntry Topic::getEntry (const std::string& infoId)
    {
        return JournalEntry (mTopic, infoId);
    }
}
