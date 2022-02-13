#include "quest.hpp"

#include <algorithm>

#include <components/esm3/queststate.hpp>

#include "../mwworld/esmstore.hpp"

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

    Quest::Quest (const ESM::QuestState& state)
    : Topic (state.mTopic), mIndex (state.mState), mFinished (state.mFinished!=0)
    {}

    std::string Quest::getName() const
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (mTopic);

        for (ESM::Dialogue::InfoContainer::const_iterator iter (dialogue->mInfo.begin());
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
        // The index must be set even if no related journal entry was found
        mIndex = index;
    }

    bool Quest::isFinished() const
    {
        return mFinished;
    }

    void Quest::setFinished(bool finished)
    {
        mFinished = finished;
    }

    bool Quest::addEntry (const JournalEntry& entry)
    {
        const ESM::Dialogue *dialogue =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find (entry.mTopic);

        auto info = std::find_if(dialogue->mInfo.begin(), dialogue->mInfo.end(), [&](const auto& info) { return info.mId == entry.mInfoId; });

        if (info == dialogue->mInfo.end() || info->mData.mJournalIndex == -1)
            throw std::runtime_error ("unknown journal entry for topic " + mTopic);

        if (info->mQuestStatus == ESM::DialInfo::QS_Finished || info->mQuestStatus == ESM::DialInfo::QS_Restart)
            mFinished = info->mQuestStatus == ESM::DialInfo::QS_Finished;

        if (info->mData.mJournalIndex > mIndex)
            mIndex = info->mData.mJournalIndex;

        for (TEntryIter iter (mEntries.begin()); iter!=mEntries.end(); ++iter)
            if (iter->mInfoId==entry.mInfoId)
                return info->mQuestStatus == ESM::DialInfo::QS_Restart;

        mEntries.push_back (entry); // we want slicing here
        return info->mQuestStatus == ESM::DialInfo::QS_Restart;
    }

    void Quest::write (ESM::QuestState& state) const
    {
        state.mTopic = getTopic();
        state.mState = mIndex;
        state.mFinished = mFinished;
    }
}
