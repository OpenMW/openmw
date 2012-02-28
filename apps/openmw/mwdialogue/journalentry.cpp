
#include "journalentry.hpp"

#include <stdexcept>

#include <components/esm_store/store.hpp>

#include "../mwworld/world.hpp"

namespace MWDialogue
{
    JournalEntry::JournalEntry() {}

    JournalEntry::JournalEntry (const std::string& topic, const std::string& infoId)
    : mTopic (topic), mInfoId (infoId)
    {}

    std::string JournalEntry::getText (const ESMS::ESMStore& store) const
    {
        const ESM::Dialogue *dialogue = store.dialogs.find (mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->id==mInfoId)
                return iter->response;

        throw std::runtime_error ("unknown info ID " + mInfoId + " for topic " + mTopic);
    }

    JournalEntry JournalEntry::makeFromQuest (const std::string& topic, int index,
        const MWWorld::World& world)
    {
        return JournalEntry (topic, idFromIndex (topic, index, world));
    }

    std::string JournalEntry::idFromIndex (const std::string& topic, int index,
        const MWWorld::World& world)
    {
        const ESM::Dialogue *dialogue = world.getStore().dialogs.find (topic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->data.disposition==index) /// \todo cleanup info structure
            {
                return iter->id;
            }

        throw std::runtime_error ("unknown journal index for topic " + topic);
    }

    StampedJournalEntry::StampedJournalEntry()
    : mDay (0), mMonth (0), mDayOfMonth (0)
    {}

    StampedJournalEntry::StampedJournalEntry (const std::string& topic, const std::string& infoId,
        int day, int month, int dayOfMonth)
    : JournalEntry (topic, infoId), mDay (day), mMonth (month), mDayOfMonth (dayOfMonth)
    {}

    StampedJournalEntry StampedJournalEntry::makeFromQuest (const std::string& topic, int index,
        const MWWorld::World& world)
    {
        int day = world.getGlobalVariable ("dayspassed").mLong;
        int month = world.getGlobalVariable ("day").mLong;
        int dayOfMonth = world.getGlobalVariable ("month").mLong;

        return StampedJournalEntry (topic, idFromIndex (topic, index, world), day, month, dayOfMonth);
    }
}
