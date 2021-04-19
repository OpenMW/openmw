#include "journalviewmodel.hpp"

#include <map>
#include <sstream>

#include <MyGUI_LanguageManager.h>

#include <components/translation/translation.hpp>
#include <components/misc/stringops.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwdialogue/keywordsearch.hpp"

namespace MWGui {

struct JournalViewModelImpl;

struct JournalViewModelImpl : JournalViewModel
{
    typedef MWDialogue::KeywordSearch <std::string, intptr_t> KeywordSearchT;

    mutable bool             mKeywordSearchLoaded;
    mutable KeywordSearchT mKeywordSearch;

    JournalViewModelImpl ()
    {
        mKeywordSearchLoaded = false;
    }

    virtual ~JournalViewModelImpl ()
    {
    }

    /// \todo replace this nasty BS
    static Utf8Span toUtf8Span (std::string const & str)
    {
        if (str.size () == 0)
            return Utf8Span (Utf8Point (nullptr), Utf8Point (nullptr));

        Utf8Point point = reinterpret_cast <Utf8Point> (str.c_str ());

        return Utf8Span (point, point + str.size ());
    }

    void load () override
    {
    }

    void unload () override
    {
        mKeywordSearch.clear ();
        mKeywordSearchLoaded = false;
    }

    void ensureKeyWordSearchLoaded () const
    {
        if (!mKeywordSearchLoaded)
        {
            MWBase::Journal * journal = MWBase::Environment::get().getJournal();

            for(MWBase::Journal::TTopicIter i = journal->topicBegin(); i != journal->topicEnd (); ++i)
                mKeywordSearch.seed (i->first, intptr_t (&i->second));

            mKeywordSearchLoaded = true;
        }
    }

    bool isEmpty () const override
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        return journal->begin () == journal->end ();
    }

    template <typename t_iterator, typename Interface>
    struct BaseEntry : Interface
    {
        typedef t_iterator iterator_t;

        iterator_t                      itr;
        JournalViewModelImpl const *    mModel;

        BaseEntry (JournalViewModelImpl const * model, iterator_t itr) :
            itr (itr), mModel (model), loaded (false)
        {}

        virtual ~BaseEntry () {}

        mutable bool loaded;
        mutable std::string utf8text;

        typedef std::pair<size_t, size_t> Range;

        // hyperlinks in @link# notation
        mutable std::map<Range, intptr_t> mHyperLinks;

        virtual std::string getText () const = 0;

        void ensureLoaded () const
        {
            if (!loaded)
            {
                mModel->ensureKeyWordSearchLoaded ();

                utf8text = getText ();

                size_t pos_end = 0;
                for(;;)
                {
                    size_t pos_begin = utf8text.find('@');
                    if (pos_begin != std::string::npos)
                        pos_end = utf8text.find('#', pos_begin);

                    if (pos_begin != std::string::npos && pos_end != std::string::npos)
                    {
                        std::string link = utf8text.substr(pos_begin + 1, pos_end - pos_begin - 1);
                        const char specialPseudoAsteriskCharacter = 127;
                        std::replace(link.begin(), link.end(), specialPseudoAsteriskCharacter, '*');
                        std::string topicName = MWBase::Environment::get().getWindowManager()->
                                getTranslationDataStorage().topicStandardForm(link);

                        std::string displayName = link;
                        while (displayName[displayName.size()-1] == '*')
                            displayName.erase(displayName.size()-1, 1);

                        utf8text.replace(pos_begin, pos_end+1-pos_begin, displayName);

                        intptr_t value = 0;
                        if (mModel->mKeywordSearch.containsKeyword(topicName, value))
                            mHyperLinks[std::make_pair(pos_begin, pos_begin+displayName.size())] = value;
                    }
                    else
                        break;
                }

                loaded = true;
            }
        }

        Utf8Span body () const override
        {
            ensureLoaded ();

            return toUtf8Span (utf8text);
        }

        void visitSpans (std::function < void (TopicId, size_t, size_t)> visitor) const override
        {
            ensureLoaded ();
            mModel->ensureKeyWordSearchLoaded ();

            if (mHyperLinks.size() && MWBase::Environment::get().getWindowManager()->getTranslationDataStorage().hasTranslation())
            {
                size_t formatted = 0; // points to the first character that is not laid out yet
                for (std::map<Range, intptr_t>::const_iterator it = mHyperLinks.begin(); it != mHyperLinks.end(); ++it)
                {
                    intptr_t topicId = it->second;
                    if (formatted < it->first.first)
                        visitor (0, formatted, it->first.first);
                    visitor (topicId, it->first.first, it->first.second);
                    formatted = it->first.second;
                }
                if (formatted < utf8text.size())
                    visitor (0, formatted, utf8text.size());
            }
            else
            {
                std::vector<KeywordSearchT::Match> matches;
                mModel->mKeywordSearch.highlightKeywords(utf8text.begin(), utf8text.end(), matches);

                std::string::const_iterator i = utf8text.begin ();
                for (std::vector<KeywordSearchT::Match>::const_iterator it = matches.begin(); it != matches.end(); ++it)
                {
                    const KeywordSearchT::Match& match = *it;

                    if (i != match.mBeg)
                        visitor (0, i - utf8text.begin (), match.mBeg - utf8text.begin ());

                    visitor (match.mValue, match.mBeg - utf8text.begin (), match.mEnd - utf8text.begin ());

                    i = match.mEnd;
                }

                if (i != utf8text.end ())
                    visitor (0, i - utf8text.begin (), utf8text.size ());
            }
        }

    };

    void visitQuestNames (bool active_only, std::function <void (const std::string&, bool)> visitor) const override
    {
        MWBase::Journal * journal = MWBase::Environment::get ().getJournal ();

        std::set<std::string> visitedQuests;

        // Note that for purposes of the journal GUI, quests are identified by the name, not the ID, so several
        // different quest IDs can end up in the same quest log. A quest log should be considered finished
        // when any quest ID in that log is finished.
        for (MWBase::Journal::TQuestIter i = journal->questBegin (); i != journal->questEnd (); ++i)
        {
            const MWDialogue::Quest& quest = i->second;

            bool isFinished = false;
            for (MWBase::Journal::TQuestIter j = journal->questBegin (); j != journal->questEnd (); ++j)
            {
                if (quest.getName() == j->second.getName() && j->second.isFinished())
                    isFinished = true;
            }

            if (active_only && isFinished)
                continue;

            // Unfortunately Morrowind.esm has no quest names, since the quest book was added with tribunal.
            // Note that even with Tribunal, some quests still don't have quest names. I'm assuming those are not supposed
            // to appear in the quest book.
            if (!quest.getName().empty())
            {
                // Don't list the same quest name twice
                if (visitedQuests.find(quest.getName()) != visitedQuests.end())
                    continue;

                visitor (quest.getName(), isFinished);

                visitedQuests.insert(quest.getName());
            }
        }
    }

    template <typename iterator_t>
    struct JournalEntryImpl : BaseEntry <iterator_t, JournalEntry>
    {
        using BaseEntry <iterator_t, JournalEntry>::itr;

        mutable std::string timestamp_buffer;

        JournalEntryImpl (JournalViewModelImpl const * model, iterator_t itr) :
            BaseEntry <iterator_t, JournalEntry> (model, itr)
        {}

        std::string getText () const override
        {
            return itr->getText();
        }

        Utf8Span timestamp () const override
        {
            if (timestamp_buffer.empty ())
            {
                std::string dayStr = MyGUI::LanguageManager::getInstance().replaceTags("#{sDay}");

                std::ostringstream os;

                os
                    << itr->mDayOfMonth << ' '
                    << MWBase::Environment::get().getWorld()->getMonthName (itr->mMonth)
                    << " (" << dayStr << " " << (itr->mDay) << ')';

                timestamp_buffer = os.str ();
            }

            return toUtf8Span (timestamp_buffer);
        }
    };

    void visitJournalEntries (const std::string& questName, std::function <void (JournalEntry const &)> visitor) const override
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        if (!questName.empty())
        {
            std::vector<MWDialogue::Quest const*> quests;
            for (MWBase::Journal::TQuestIter questIt = journal->questBegin(); questIt != journal->questEnd(); ++questIt)
            {
                if (Misc::StringUtils::ciEqual(questIt->second.getName(), questName))
                    quests.push_back(&questIt->second);
            }

            for(MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end (); ++i)
            {
                for (std::vector<MWDialogue::Quest const*>::iterator questIt = quests.begin(); questIt != quests.end(); ++questIt)
                {
                    MWDialogue::Quest const* quest = *questIt;
                    for (MWDialogue::Topic::TEntryIter j = quest->begin (); j != quest->end (); ++j)
                    {
                        if (i->mInfoId == j->mInfoId)
                            visitor (JournalEntryImpl <MWBase::Journal::TEntryIter> (this, i));
                    }
                }
            }
        }
        else
        {
            for(MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end (); ++i)
                visitor (JournalEntryImpl <MWBase::Journal::TEntryIter> (this, i));
        }
    }

    void visitTopicName (TopicId topicId, std::function <void (Utf8Span)> visitor) const override
    {
        MWDialogue::Topic const & topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);
        visitor (toUtf8Span (topic.getName()));
    }

    void visitTopicNamesStartingWith (Utf8Stream::UnicodeChar character, std::function < void (const std::string&) > visitor) const override
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            Utf8Stream stream (i->first.c_str());
            Utf8Stream::UnicodeChar first = Misc::StringUtils::toLowerUtf8(stream.peek());

            if (first != Misc::StringUtils::toLowerUtf8(character))
                continue;

            visitor (i->second.getName());
        }
    }

    struct TopicEntryImpl : BaseEntry <MWDialogue::Topic::TEntryIter, TopicEntry>
    {
        MWDialogue::Topic const & mTopic;

        TopicEntryImpl (JournalViewModelImpl const * model, MWDialogue::Topic const & topic, iterator_t itr) :
            BaseEntry (model, itr), mTopic (topic)
        {}

        std::string getText () const override
        {
            return  itr->getText();
        }

        Utf8Span source () const override
        {
            return toUtf8Span (itr->mActorName);
        }

    };

    void visitTopicEntries (TopicId topicId, std::function <void (TopicEntry const &)> visitor) const override
    {
        typedef MWDialogue::Topic::TEntryIter iterator_t;

        MWDialogue::Topic const & topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        for (iterator_t i = topic.begin (); i != topic.end (); ++i)
            visitor (TopicEntryImpl (this, topic, i));
    }
};

JournalViewModel::Ptr JournalViewModel::create ()
{
    return std::make_shared <JournalViewModelImpl> ();
}

}
