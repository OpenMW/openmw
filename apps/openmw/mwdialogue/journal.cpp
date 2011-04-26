
#include "journal.hpp"

#include "../mwworld/environment.hpp"

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

    Journal::Journal (MWWorld::Environment& environment)
    : mEnvironment (environment)
    {}

    void Journal::addEntry (const std::string& id, int index)
    {
        StampedJournalEntry entry =
            StampedJournalEntry::makeFromQuest (id, index, *mEnvironment.mWorld);

        mJournal.push_back (entry);

        Quest& quest = getQuest (id);

        quest.addEntry (entry, *mEnvironment.mWorld); // we are doing slicing on purpose here
    }

    void Journal::setJournalIndex (const std::string& id, int index)
    {
        Quest& quest = getQuest (id);

        quest.setIndex (index, *mEnvironment.mWorld);
    }

    int Journal::getJournalIndex (const std::string& id) const
    {
        return 0;
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
}
