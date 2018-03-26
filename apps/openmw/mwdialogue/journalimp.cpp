#include "journalimp.hpp"

#include <iterator>

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/queststate.hpp>
#include <components/esm/journalentry.hpp>

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

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

    Topic& Journal::getTopic (const std::string& id)
    {
        TTopicContainer::iterator iter = mTopics.find (id);

        if (iter==mTopics.end())
        {
            std::pair<TTopicContainer::iterator, bool> result
                = mTopics.insert (std::make_pair (id, Topic (id)));

            iter = result.first;
        }

        return iter->second;
    }

    bool Journal::isThere (const std::string& topicId, const std::string& infoId) const
    {
        if (const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().search (topicId))
        {
            if (infoId.empty())
                return true;

            for (ESM::Dialogue::InfoContainer::const_iterator iter (dialogue->mInfo.begin());
                iter!=dialogue->mInfo.end(); ++iter)
                if (iter->mId == infoId)
                    return true;
        }

        return false;
    }

    Journal::Journal()
    {}

    void Journal::clear()
    {
        mJournal.clear();
        mQuests.clear();
        mTopics.clear();
    }

    void Journal::addEntry (const std::string& id, int index, const MWWorld::Ptr& actor)
    {
        // bail out if we already have heard this...
        std::string infoId = JournalEntry::idFromIndex (id, index);
        for (TEntryIter i = mJournal.begin (); i != mJournal.end (); ++i)
            if (i->mTopic == id && i->mInfoId == infoId)
            {
                if (getJournalIndex(id) < index)
                {
                    setJournalIndex(id, index);
                    MWBase::Environment::get().getWindowManager()->messageBox ("#{sJournalEntry}");
                }
                return;
            }

        StampedJournalEntry entry = StampedJournalEntry::makeFromQuest (id, index, actor);

        Quest& quest = getQuest (id);
        quest.addEntry (entry); // we are doing slicing on purpose here

        // there is no need to show empty entries in journal
        if (!entry.getText().empty())
        {
            mJournal.push_back (entry);
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sJournalEntry}");
        }
    }

    void Journal::setJournalIndex (const std::string& id, int index)
    {
        Quest& quest = getQuest (id);

        quest.setIndex (index);
    }

    void Journal::addTopic (const std::string& topicId, const std::string& infoId, const MWWorld::Ptr& actor)
    {
        Topic& topic = getTopic (topicId);

        JournalEntry entry(topicId, infoId, actor);
        entry.mActorName = actor.getClass().getName(actor);
        topic.addEntry (entry);
    }

    void Journal::removeLastAddedTopicResponse(const std::string &topicId, const std::string &actorName)
    {
        Topic& topic = getTopic (topicId);

        topic.removeLastAddedResponse(actorName);

        if (topic.begin() == topic.end())
            mTopics.erase(mTopics.find(topicId)); // All responses removed -> remove topic
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

    int Journal::countSavedGameRecords() const
    {
        int count = static_cast<int> (mQuests.size());

        for (TQuestIter iter (mQuests.begin()); iter!=mQuests.end(); ++iter)
            count += std::distance (iter->second.begin(), iter->second.end());

        count += std::distance (mJournal.begin(), mJournal.end());

        for (TTopicIter iter (mTopics.begin()); iter!=mTopics.end(); ++iter)
            count += std::distance (iter->second.begin(), iter->second.end());

        return count;
    }

    void Journal::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (TQuestIter iter (mQuests.begin()); iter!=mQuests.end(); ++iter)
        {
            const Quest& quest = iter->second;

            ESM::QuestState state;
            quest.write (state);
            writer.startRecord (ESM::REC_QUES);
            state.save (writer);
            writer.endRecord (ESM::REC_QUES);

            for (Topic::TEntryIter entryIter (quest.begin()); entryIter!=quest.end(); ++entryIter)
            {
                ESM::JournalEntry entry;
                entry.mType = ESM::JournalEntry::Type_Quest;
                entry.mTopic = quest.getTopic();
                entryIter->write (entry);
                writer.startRecord (ESM::REC_JOUR);
                entry.save (writer);
                writer.endRecord (ESM::REC_JOUR);
            }
        }

        for (TEntryIter iter (mJournal.begin()); iter!=mJournal.end(); ++iter)
        {
            ESM::JournalEntry entry;
            entry.mType = ESM::JournalEntry::Type_Journal;
            iter->write (entry);
            writer.startRecord (ESM::REC_JOUR);
            entry.save (writer);
            writer.endRecord (ESM::REC_JOUR);
        }

        for (TTopicIter iter (mTopics.begin()); iter!=mTopics.end(); ++iter)
        {
            const Topic& topic = iter->second;

            for (Topic::TEntryIter entryIter (topic.begin()); entryIter!=topic.end(); ++entryIter)
            {
                ESM::JournalEntry entry;
                entry.mType = ESM::JournalEntry::Type_Topic;
                entry.mTopic = topic.getTopic();
                entryIter->write (entry);
                writer.startRecord (ESM::REC_JOUR);
                entry.save (writer);
                writer.endRecord (ESM::REC_JOUR);
            }
        }
    }

    void Journal::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        if (type==ESM::REC_JOUR || type==ESM::REC_JOUR_LEGACY)
        {
            ESM::JournalEntry record;
            record.load (reader);

            if (isThere (record.mTopic, record.mInfo))
                switch (record.mType)
                {
                    case ESM::JournalEntry::Type_Quest:

                        getQuest (record.mTopic).insertEntry (record);
                        break;

                    case ESM::JournalEntry::Type_Journal:

                        mJournal.push_back (record);
                        break;

                    case ESM::JournalEntry::Type_Topic:

                        getTopic (record.mTopic).insertEntry (record);
                        break;
                }
        }
        else if (type==ESM::REC_QUES)
        {
            ESM::QuestState record;
            record.load (reader);

            if (isThere (record.mTopic))
            {
                std::pair<TQuestContainer::iterator, bool> result = mQuests.insert (std::make_pair (record.mTopic, record));
                // reapply quest index, this is to handle users upgrading from only
                // Morrowind.esm (no quest states) to Morrowind.esm + Tribunal.esm
                result.first->second.setIndex(record.mState);
            }
        }
    }
}
