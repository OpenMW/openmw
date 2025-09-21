#include "journalviewmodel.hpp"

#include <map>

#include <MyGUI_LanguageManager.h>

#include <components/misc/strings/algorithm.hpp>
#include <components/translation/translation.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwdialogue/keywordsearch.hpp"
#include "../mwworld/datetimemanager.hpp"

namespace MWGui
{

    struct JournalViewModelImpl;

    struct JournalViewModelImpl : JournalViewModel
    {
        typedef MWDialogue::KeywordSearch<intptr_t> KeywordSearchT;

        mutable bool mKeywordSearchLoaded;
        mutable KeywordSearchT mKeywordSearch;

        JournalViewModelImpl() { mKeywordSearchLoaded = false; }

        virtual ~JournalViewModelImpl() = default;

        /// \todo replace this nasty BS
        static Utf8Span toUtf8Span(std::string_view str)
        {
            if (str.empty())
                return Utf8Span(Utf8Point(nullptr), Utf8Point(nullptr));

            Utf8Point point = reinterpret_cast<Utf8Point>(str.data());

            return Utf8Span(point, point + str.size());
        }

        void load() override {}

        void unload() override
        {
            mKeywordSearch.clear();
            mKeywordSearchLoaded = false;
        }

        void ensureKeyWordSearchLoaded() const
        {
            if (!mKeywordSearchLoaded)
            {
                MWBase::Journal* journal = MWBase::Environment::get().getJournal();

                for (const auto& [_, topic] : journal->getTopics())
                    mKeywordSearch.seed(topic.getName(), intptr_t(&topic));

                mKeywordSearchLoaded = true;
            }
        }

        bool isEmpty() const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            return journal->getEntries().empty();
        }

        template <typename EntryType, typename Interface>
        struct BaseEntry : Interface
        {
            const EntryType* mEntry;
            JournalViewModelImpl const* mModel;

            BaseEntry(JournalViewModelImpl const* model, const EntryType& entry)
                : mEntry(&entry)
                , mModel(model)
                , loaded(false)
            {
            }

            virtual ~BaseEntry() = default;

            mutable bool loaded;
            mutable std::string utf8text;

            typedef std::pair<size_t, size_t> Range;

            // hyperlinks in @link# notation
            mutable std::map<Range, intptr_t> mHyperLinks;

            virtual std::string getText() const = 0;

            void ensureLoaded() const
            {
                if (!loaded)
                {
                    mModel->ensureKeyWordSearchLoaded();

                    utf8text = getText();

                    size_t posEnd = 0;
                    for (;;)
                    {
                        const size_t posBegin = utf8text.find('@');
                        if (posBegin != std::string::npos)
                            posEnd = utf8text.find('#', posBegin);

                        if (posBegin != std::string::npos && posEnd != std::string::npos)
                        {
                            std::string link = utf8text.substr(posBegin + 1, posEnd - posBegin - 1);
                            const char specialPseudoAsteriskCharacter = 127;
                            std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                            std::string_view topicName = MWBase::Environment::get()
                                                             .getWindowManager()
                                                             ->getTranslationDataStorage()
                                                             .topicStandardForm(link);

                            std::string displayName = link;
                            while (displayName[displayName.size() - 1] == '*')
                                displayName.erase(displayName.size() - 1, 1);

                            utf8text.replace(posBegin, posEnd + 1 - posBegin, displayName);

                            intptr_t value = 0;
                            if (mModel->mKeywordSearch.containsKeyword(topicName, value))
                                mHyperLinks[std::make_pair(posBegin, posBegin + displayName.size())] = value;
                        }
                        else
                            break;
                    }

                    loaded = true;
                }
            }

            Utf8Span body() const override
            {
                ensureLoaded();

                return toUtf8Span(utf8text);
            }

            void visitSpans(std::function<void(TopicId, size_t, size_t)> visitor) const override
            {
                ensureLoaded();
                mModel->ensureKeyWordSearchLoaded();

                if (mHyperLinks.size()
                    && MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
                {
                    size_t formatted = 0; // points to the first character that is not laid out yet
                    for (std::map<Range, intptr_t>::const_iterator it = mHyperLinks.begin(); it != mHyperLinks.end();
                         ++it)
                    {
                        intptr_t topicId = it->second;
                        if (formatted < it->first.first)
                            visitor(0, formatted, it->first.first);
                        visitor(topicId, it->first.first, it->first.second);
                        formatted = it->first.second;
                    }
                    if (formatted < utf8text.size())
                        visitor(0, formatted, utf8text.size());
                }
                else
                {
                    std::vector<KeywordSearchT::Match> matches;
                    mModel->mKeywordSearch.highlightKeywords(utf8text.begin(), utf8text.end(), matches);

                    std::string::const_iterator i = utf8text.begin();
                    for (const KeywordSearchT::Match& match : matches)
                    {
                        if (i != match.mBeg)
                            visitor(0, i - utf8text.begin(), match.mBeg - utf8text.begin());

                        visitor(match.mValue, match.mBeg - utf8text.begin(), match.mEnd - utf8text.begin());

                        i = match.mEnd;
                    }

                    if (i != utf8text.end())
                        visitor(0, i - utf8text.begin(), utf8text.size());
                }
            }
        };

        void visitQuestNames(bool activeOnly, std::function<void(std::string_view, bool)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            std::set<std::string_view, Misc::StringUtils::CiComp> visitedQuests;

            // Note that for purposes of the journal GUI, quests are identified by the name, not the ID, so several
            // different quest IDs can end up in the same quest log. A quest log should be considered finished
            // when any quest ID in that log is finished.
            for (const auto& [_, quest] : journal->getQuests())
            {
                // Unfortunately Morrowind.esm has no quest names, since the quest book was added with tribunal.
                // Note that even with Tribunal, some quests still don't have quest names. I'm assuming those are not
                // supposed to appear in the quest book.
                const std::string_view questName = quest.getName();
                if (questName.empty())
                    continue;
                // Don't list the same quest name twice
                if (!visitedQuests.insert(questName).second)
                    continue;

                bool isFinished = std::ranges::find_if(journal->getQuests(), [&](const auto& pair) {
                    return pair.second.isFinished() && Misc::StringUtils::ciEqual(questName, pair.second.getName());
                }) != journal->getQuests().end();

                if (activeOnly && isFinished)
                    continue;

                visitor(questName, isFinished);
            }
        }

        struct JournalEntryImpl : BaseEntry<MWDialogue::StampedJournalEntry, JournalEntry>
        {
            mutable std::string timestamp_buffer;

            JournalEntryImpl(JournalViewModelImpl const* model, const MWDialogue::StampedJournalEntry& entry)
                : BaseEntry(model, entry)
            {
            }

            std::string getText() const override { return mEntry->getText(); }

            Utf8Span timestamp() const override
            {
                if (timestamp_buffer.empty())
                {
                    std::string dayStr = MyGUI::LanguageManager::getInstance().replaceTags("#{sDay}");

                    std::ostringstream os;

                    os << mEntry->mDayOfMonth << ' '
                       << MWBase::Environment::get().getWorld()->getTimeManager()->getMonthName(mEntry->mMonth) << " ("
                       << dayStr << " " << (mEntry->mDay) << ')';

                    timestamp_buffer = os.str();
                }

                return toUtf8Span(timestamp_buffer);
            }
        };

        void visitJournalEntries(
            std::string_view questName, std::function<void(JournalEntry const&)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            if (!questName.empty())
            {
                std::vector<MWDialogue::Quest const*> quests;
                for (const auto& [_, quest] : journal->getQuests())
                {
                    if (Misc::StringUtils::ciEqual(quest.getName(), questName))
                        quests.push_back(&quest);
                }

                for (const MWDialogue::StampedJournalEntry& journalEntry : journal->getEntries())
                {
                    for (const MWDialogue::Quest* quest : quests)
                    {
                        if (quest->getTopic() != journalEntry.mTopic)
                            continue;
                        for (const MWDialogue::Entry& questEntry : *quest)
                        {
                            if (journalEntry.mInfoId == questEntry.mInfoId)
                            {
                                visitor(JournalEntryImpl(this, journalEntry));
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                for (const MWDialogue::StampedJournalEntry& journalEntry : journal->getEntries())
                    visitor(JournalEntryImpl(this, journalEntry));
            }
        }

        void visitTopicName(TopicId topicId, std::function<void(Utf8Span)> visitor) const override
        {
            MWDialogue::Topic const& topic = *reinterpret_cast<MWDialogue::Topic const*>(topicId);
            visitor(toUtf8Span(topic.getName()));
        }

        void visitTopicNamesStartingWith(
            Utf8Stream::UnicodeChar character, std::function<void(std::string_view)> visitor) const override
        {
            MWBase::Journal* journal = MWBase::Environment::get().getJournal();

            for (const auto& [_, topic] : journal->getTopics())
            {
                Utf8Stream stream(topic.getName());
                Utf8Stream::UnicodeChar first = Utf8Stream::toLowerUtf8(stream.peek());

                if (first != Utf8Stream::toLowerUtf8(character))
                    continue;

                visitor(topic.getName());
            }
        }

        struct TopicEntryImpl : BaseEntry<MWDialogue::Entry, TopicEntry>
        {
            MWDialogue::Topic const& mTopic;

            TopicEntryImpl(
                JournalViewModelImpl const* model, MWDialogue::Topic const& topic, const MWDialogue::Entry& entry)
                : BaseEntry(model, entry)
                , mTopic(topic)
            {
            }

            std::string getText() const override { return mEntry->getText(); }

            Utf8Span source() const override { return toUtf8Span(mEntry->mActorName); }
        };

        void visitTopicEntries(TopicId topicId, std::function<void(TopicEntry const&)> visitor) const override
        {
            MWDialogue::Topic const& topic = *reinterpret_cast<MWDialogue::Topic const*>(topicId);

            for (const MWDialogue::Entry& entry : topic)
                visitor(TopicEntryImpl(this, topic, entry));
        }
    };

    JournalViewModel::Ptr JournalViewModel::create()
    {
        return std::make_shared<JournalViewModelImpl>();
    }

}
