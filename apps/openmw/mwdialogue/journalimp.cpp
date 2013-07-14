
#include "journalimp.hpp"

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/messagebox.hpp"

namespace MWDialogue
{
    Quest& Journal::getQuest (const std::string& id)
    {
        TQuestContainer::iterator iter = mQuests.find (id);

        if (iter==mQuests.end())
        {
            std::pair<TQuestContainer::iterator, bool> result =
                mQuests.insert (std::make_pair (id, Quest (id)));

            iter = result.first;
        }

        return iter->second;
    }

    Journal::Journal()
    {}

    void Journal::clear()
    {
        mJournal.clear();
        mQuests.clear();
        mTopics.clear();
    }

    void Journal::addEntry (const std::string& id, int index)
    {
        // bail out of we already have heard this...
        std::string infoId = JournalEntry::idFromIndex (id, index);
        for (TEntryIter i = mJournal.begin (); i != mJournal.end (); ++i)
            if (i->mTopic == id && i->mInfoId == infoId)
                return;

        StampedJournalEntry entry = StampedJournalEntry::makeFromQuest (id, index);

        mJournal.push_back (entry);

        Quest& quest = getQuest (id);

        quest.addEntry (entry); // we are doing slicing on purpose here

        std::vector<std::string> empty;
        std::string notification = "#{sJournalEntry}";
        MWBase::Environment::get().getWindowManager()->messageBox (notification, empty);
    }

    void Journal::setJournalIndex (const std::string& id, int index)
    {
        Quest& quest = getQuest (id);

        quest.setIndex (index);
    }

    void Journal::addTopic (const std::string& topicId, const std::string& infoId)
    {
        TTopicContainer::iterator iter = mTopics.find (topicId);

        if (iter==mTopics.end())
        {
            std::pair<TTopicContainer::iterator, bool> result
                = mTopics.insert (std::make_pair (topicId, Topic (topicId)));

            iter = result.first;
        }

        iter->second.addEntry (JournalEntry (topicId, infoId));
    }

    int Journal::getJournalIndex (const std::string& id) const
    {
        TQuestContainer::const_iterator iter = mQuests.find (id);

        if (iter==mQuests.end())
            return 0;

        return iter->second.getIndex();
    }

    Journal::TEntryIter Journal::begin() const
    {
        return mJournal.begin();
    }

    Journal::TEntryIter Journal::end() const
    {
        return mJournal.end();
    }

    Journal::TQuestIter Journal::questBegin() const
    {
        return mQuests.begin();
    }

    Journal::TQuestIter Journal::questEnd() const
    {
        return mQuests.end();
    }

    Journal::TTopicIter Journal::topicBegin() const
    {
        return mTopics.begin();
    }

    Journal::TTopicIter Journal::topicEnd() const
    {
        return mTopics.end();
    }
}
