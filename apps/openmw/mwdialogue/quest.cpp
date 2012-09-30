
#include "quest.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWDialogue
{
    Quest::Quest()
    : Topic(), mIndex (0), mFinished (false)
    {}

    Quest::Quest (const std::string& topic)
    : Topic (topic), mIndex (0), mFinished (false)
    {}

    const std::string Quest::getName() const
    {
        const ESM::Dialogue *dialogue = MWBase::Environment::get().getWorld()->getStore().dialogs.find (mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mQuestStatus==ESM::DialInfo::QS_Name)
                return iter->mResponse;

        return "";
    }

    int Quest::getIndex() const
    {
        return mIndex;
    }

    void Quest::setIndex (int index)
    {
        const ESM::Dialogue *dialogue = MWBase::Environment::get().getWorld()->getStore().dialogs.find (mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mData.mDisposition==index && iter->mQuestStatus!=ESM::DialInfo::QS_Name)
            {
                mIndex = index;

                if (iter->mQuestStatus==ESM::DialInfo::QS_Finished)
                    mFinished = true;
                else if (iter->mQuestStatus==ESM::DialInfo::QS_Restart)
                    mFinished = false;

                return;
            }

        throw std::runtime_error ("unknown journal index for topic " + mTopic);
    }

    bool Quest::isFinished() const
    {
        return mFinished;
    }

    void Quest::addEntry (const JournalEntry& entry)
    {
        int index = -1;

        const ESM::Dialogue *dialogue = MWBase::Environment::get().getWorld()->getStore().dialogs.find (entry.mTopic);

        for (std::vector<ESM::DialInfo>::const_iterator iter (dialogue->mInfo.begin());
            iter!=dialogue->mInfo.end(); ++iter)
            if (iter->mId == entry.mInfoId)
            {
                index = iter->mData.mDisposition; /// \todo cleanup info structure
                break;
            }

        if (index==-1)
            throw std::runtime_error ("unknown journal entry for topic " + mTopic);

        setIndex (index);

        for (TEntryIter iter (mEntries.begin()); iter!=mEntries.end(); ++iter)
            if (*iter==entry.mInfoId)
                return;

        mEntries.push_back (entry.mInfoId);
    }
}
