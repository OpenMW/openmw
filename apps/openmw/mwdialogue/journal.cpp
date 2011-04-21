
#include "journal.hpp"

#include "../mwworld/environment.hpp"

#include <iostream>
namespace MWDialogue
{
    Journal::Journal (MWWorld::Environment& environment)
    : mEnvironment (environment)
    {}

    void Journal::addEntry (const std::string& id, int index)
    {
        mJournal.push_back (JournalEntry::makeFromQuest (id, index, *mEnvironment.mWorld));
        std::cout << "journal: " << id << " at " << index << std::endl;
    }

    void Journal::setJournalIndex (const std::string& id, int index)
    {
        std::cout << "journal (no entry): " << id << " at " << index << std::endl;
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
}
