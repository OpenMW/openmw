#ifndef GAME_MWDIALOG_QUEST_H
#define GAME_MWDIALOG_QUEST_H

#include "topic.hpp"

namespace ESM
{
    struct QuestState;
}

namespace MWDialogue
{
    /// \brief A quest in progress or a completed quest
    class Quest : public Topic
    {
            int mIndex;
            bool mFinished;

        public:

            Quest();

            Quest (const std::string& topic);

            Quest (const ESM::QuestState& state);

            std::string getName() const override;
            ///< May be an empty string

            int getIndex() const;

            void setIndex (int index);
            ///< Calling this function with a non-existent index will throw an exception.

            bool isFinished() const;

            void addEntry (const JournalEntry& entry) override;
            ///< Add entry and adjust index accordingly.
            ///
            /// \note Redundant entries are ignored, but the index is still adjusted.

            void write (ESM::QuestState& state) const;
    };
}

#endif
