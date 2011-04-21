
#include "journalentry.hpp"

#include <stdexcept>

#include <components/esm_store/store.hpp>

#include "../mwworld/world.hpp"

namespace MWDialogue
{
    JournalEntry::JournalEntry() {}

    JournalEntry::JournalEntry (int day, const std::string& topic, const std::string& infoId)
    : mDay (day), mTopic (topic), mInfoId (infoId)
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
        const ESM::Dialogue *dialogue = world.getStore().dialogs.find (topic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->data.disposition==index) /// \todo cleanup info structure
            {
                int day = world.getGlobalVariable ("dayspassed").mLong;
                return JournalEntry (day, topic, iter->id);
            }

        throw std::runtime_error ("unknown journal index for topic " + topic);
    }
}
