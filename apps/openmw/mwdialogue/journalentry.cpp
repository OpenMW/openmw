
#include "journalentry.hpp"

#include <stdexcept>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

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
            if (iter->mId == mInfoId)
                return iter->mResponse;

        throw std::runtime_error ("unknown info ID " + mInfoId + " for topic " + mTopic);
    }

    JournalEntry JournalEntry::makeFromQuest (const std::string& topic, int index)
    {
        return JournalEntry (topic, idFromIndex (topic, index));
    }

    std::string JournalEntry::idFromIndex (const std::string& topic, int index)
    {
        const ESM::Dialogue *dialogue = MWBase::Environment::get().getWorld()->getStore().dialogs.find (topic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mData.mDisposition==index) /// \todo cleanup info structure
            {
                return iter->mId;
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

    StampedJournalEntry StampedJournalEntry::makeFromQuest (const std::string& topic, int index)
    {
        int day = MWBase::Environment::get().getWorld()->getGlobalVariable ("dayspassed").mLong;
        int month = MWBase::Environment::get().getWorld()->getGlobalVariable ("day").mLong;
        int dayOfMonth = MWBase::Environment::get().getWorld()->getGlobalVariable ("month").mLong;

        return StampedJournalEntry (topic, idFromIndex (topic, index), day, month, dayOfMonth);
    }
}
