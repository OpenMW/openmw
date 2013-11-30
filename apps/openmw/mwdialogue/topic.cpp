
#include "topic.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWDialogue
{
    Topic::Topic()
    {}

    Topic::Topic (const std::string& topic)
    : mTopic (topic), mName (
      MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (topic)->mId)
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

    std::string Topic::getName() const
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

    JournalEntry Topic::getEntry (const std::string& infoId) const
    {
        return JournalEntry (mTopic, infoId);
    }
}
