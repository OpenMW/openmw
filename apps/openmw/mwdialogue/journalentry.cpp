
#include "journalentry.hpp"

#include <stdexcept>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWDialogue
{
    Entry::Entry() {}

    Entry::Entry (const std::string& topic, const std::string& infoId)
    : mInfoId (infoId)
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (topic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mId == mInfoId)
            {
                /// \todo text replacement
                mText = iter->mResponse;
                return;
            }

        throw std::runtime_error ("unknown info ID " + mInfoId + " for topic " + topic);
    }

    std::string Entry::getText() const
    {
        return mText;
    }


    JournalEntry::JournalEntry() {}

    JournalEntry::JournalEntry (const std::string& topic, const std::string& infoId)
    : Entry (topic, infoId), mTopic (topic)
    {}

    JournalEntry JournalEntry::makeFromQuest (const std::string& topic, int index)
    {
        return JournalEntry (topic, idFromIndex (topic, index));
    }

    std::string JournalEntry::idFromIndex (const std::string& topic, int index)
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (topic);

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
        int day = MWBase::Environment::get().getWorld()->getGlobalInt ("dayspassed");
        int month = MWBase::Environment::get().getWorld()->getGlobalInt ("month");
        int dayOfMonth = MWBase::Environment::get().getWorld()->getGlobalInt ("day");

        return StampedJournalEntry (topic, idFromIndex (topic, index), day, month, dayOfMonth);
    }
}
