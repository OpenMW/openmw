
#include "quest.hpp"

#include <components/esm_store/store.hpp>

#include "../mwworld/world.hpp"

namespace MWDialogue
{
    Quest::Quest()
    : mIndex (0), mFinished (false)
    {}

    Quest::Quest (const std::string& topic)
    : mTopic (topic), mIndex (0), mFinished (false)
    {}

    const std::string Quest::getName (const MWWorld::World& world) const
    {
        const ESM::Dialogue *dialogue = world.getStore().dialogs.find (mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->questStatus==ESM::DialInfo::QS_Name)
                return iter->response;

        return "";
    }

    int Quest::getIndex() const
    {
        return mIndex;
    }

    void Quest::setIndex (int index, const MWWorld::World& world)
    {
        const ESM::Dialogue *dialogue = world.getStore().dialogs.find (mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->data.disposition==index && iter->questStatus!=ESM::DialInfo::QS_Name)
            {
                mIndex = index;

                if (iter->questStatus==ESM::DialInfo::QS_Finished)
                    mFinished = true;
                else if (iter->questStatus==ESM::DialInfo::QS_Restart)
                    mFinished = false;

                return;
            }

        throw std::runtime_error ("unknown journal index for topic " + mTopic);
    }

    bool Quest::isFinished() const
    {
        return mFinished;
    }

    void Quest::addEntry (const JournalEntry& entry, const MWWorld::World& world)
    {
        int index = -1;

        const ESM::Dialogue *dialogue = world.getStore().dialogs.find (entry.mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->id==entry.mInfoId)
            {
                index = iter->data.disposition; /// \todo cleanup info structure
                break;
            }

        if (index==-1)
            throw std::runtime_error ("unknown journal entry for topic " + mTopic);

        setIndex (index, world);

        for (TEntryIter iter (mEntries.begin()); iter!=mEntries.end(); ++iter)
            if (*iter==entry.mInfoId)
                return;

        mEntries.push_back (entry.mInfoId);
    }

    Quest::TEntryIter Quest::begin()
    {
        return mEntries.begin();
    }

    Quest::TEntryIter Quest::end()
    {
        return mEntries.end();
    }

    JournalEntry Quest::getEntry (const std::string& infoId)
    {
        return JournalEntry (mTopic, infoId);
    }
}
