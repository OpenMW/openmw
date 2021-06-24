#include "journalentry.hpp"

#include <stdexcept>

#include <components/esm/journalentry.hpp>

#include <components/interpreter/defines.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include "../mwscript/interpretercontext.hpp"


namespace MWDialogue
{
    Entry::Entry (const std::string& topic, const std::string& infoId, const MWWorld::Ptr& actor)
    : mInfoId (infoId)
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (topic);

        for (ESM::Dialogue::InfoContainer::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mId == mInfoId)
            {
                if (actor.isEmpty())
                {
                    MWScript::InterpreterContext interpreterContext(nullptr, MWWorld::Ptr());
                    mText = Interpreter::fixDefinesDialog(iter->mResponse, interpreterContext);
                }
                else
                {
                    MWScript::InterpreterContext interpreterContext(&actor.getRefData().getLocals(),actor);
                    mText = Interpreter::fixDefinesDialog(iter->mResponse, interpreterContext);
                }

                return;
            }

        throw std::runtime_error ("unknown info ID " + mInfoId + " for topic " + topic);
    }

    Entry::Entry (const ESM::JournalEntry& record) : mInfoId (record.mInfo), mText (record.mText), mActorName(record.mActorName) {}

    std::string Entry::getText() const
    {
        return mText;
    }

    void Entry::write (ESM::JournalEntry& entry) const
    {
        entry.mInfo = mInfoId;
        entry.mText = mText;
        entry.mActorName = mActorName;
    }


    JournalEntry::JournalEntry (const std::string& topic, const std::string& infoId, const MWWorld::Ptr& actor)
        : Entry (topic, infoId, actor), mTopic (topic)
    {}

    JournalEntry::JournalEntry (const ESM::JournalEntry& record)
        : Entry (record), mTopic (record.mTopic)
    {}

    void JournalEntry::write (ESM::JournalEntry& entry) const
    {
        Entry::write (entry);
        entry.mTopic = mTopic;
    }

    JournalEntry JournalEntry::makeFromQuest (const std::string& topic, int index)
    {
        return JournalEntry (topic, idFromIndex (topic, index), MWWorld::Ptr());
    }

    std::string JournalEntry::idFromIndex (const std::string& topic, int index)
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (topic);

        for (ESM::Dialogue::InfoContainer::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mData.mJournalIndex==index)
            {
                return iter->mId;
            }

        throw std::runtime_error ("unknown journal index for topic " + topic);
    }


    StampedJournalEntry::StampedJournalEntry()
    : mDay (0), mMonth (0), mDayOfMonth (0)
    {}

    StampedJournalEntry::StampedJournalEntry (const std::string& topic, const std::string& infoId,
        int day, int month, int dayOfMonth, const MWWorld::Ptr& actor)
    : JournalEntry (topic, infoId, actor), mDay (day), mMonth (month), mDayOfMonth (dayOfMonth)
    {}

    StampedJournalEntry::StampedJournalEntry (const ESM::JournalEntry& record)
    : JournalEntry (record), mDay (record.mDay), mMonth (record.mMonth),
      mDayOfMonth (record.mDayOfMonth)
    {}

    void StampedJournalEntry::write (ESM::JournalEntry& entry) const
    {
        JournalEntry::write (entry);
        entry.mDay = mDay;
        entry.mMonth = mMonth;
        entry.mDayOfMonth = mDayOfMonth;
    }

    StampedJournalEntry StampedJournalEntry::makeFromQuest (const std::string& topic, int index, const MWWorld::Ptr& actor)
    {
        int day = MWBase::Environment::get().getWorld()->getGlobalInt ("dayspassed");
        int month = MWBase::Environment::get().getWorld()->getGlobalInt ("month");
        int dayOfMonth = MWBase::Environment::get().getWorld()->getGlobalInt ("day");

        return StampedJournalEntry (topic, idFromIndex (topic, index), day, month, dayOfMonth, actor);
    }
}
