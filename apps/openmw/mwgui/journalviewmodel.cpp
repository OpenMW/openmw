#include "journalviewmodel.hpp"

#include <map>
#include <sstream>
#include <boost/make_shared.hpp>

#include <MyGUI_LanguageManager.h>

#include <components/misc/utf8stream.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwdialogue/journalentry.hpp"

#include "keywordsearch.hpp"

namespace MWGui {

struct JournalViewModelImpl;

struct JournalViewModelImpl : JournalViewModel
{
    typedef KeywordSearch <std::string, intptr_t> KeywordSearchT;

    mutable bool             mKeywordSearchLoaded;
    mutable KeywordSearchT mKeywordSearch;

    std::locale mLocale;

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
            return Utf8Span (Utf8Point (NULL), Utf8Point (NULL));

        Utf8Point point = reinterpret_cast <Utf8Point> (str.c_str ());

        return Utf8Span (point, point + str.size ());
    }

    void load ()
    {
    }

    void unload ()
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

    wchar_t tolower (wchar_t ch) const { return std::tolower (ch, mLocale); }

    bool isEmpty () const
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
            mModel (model), itr (itr), loaded (false)
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

                size_t pos_begin, pos_end;
                for(;;)
                {
                    pos_begin = utf8text.find('@');
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

                        intptr_t value;
                        if (mModel->mKeywordSearch.containsKeyword(topicName, value))
                            mHyperLinks[std::make_pair(pos_begin, pos_begin+displayName.size())] = value;
                    }
                    else
                        break;
                }

                loaded = true;
            }
        }

        Utf8Span body () const
        {
            ensureLoaded ();

            return toUtf8Span (utf8text);
        }

        void visitSpans (boost::function < void (TopicId, size_t, size_t)> visitor) const
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
                std::string::const_iterator i = utf8text.begin ();

                KeywordSearchT::Match match;

                while (i != utf8text.end () && mModel->mKeywordSearch.search (i, utf8text.end (), match, utf8text.begin()))
                {
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

    void visitQuestNames (bool active_only, boost::function <void (QuestId, Utf8Span)> visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get ().getJournal ();

        for (MWBase::Journal::TQuestIter i = journal->questBegin (); i != journal->questEnd (); ++i)
        {
            if (active_only && i->second.isFinished ())
                continue;

            const MWDialogue::Quest& quest = i->second;
            // Unfortunately Morrowind.esm has no quest names, since the quest book was added with tribunal.
            // Note that even with Tribunal, some quests still don't have quest names. I'm assuming those are not supposed
            // to appear in the quest book.
            if (!quest.getName().empty())
                visitor (reinterpret_cast <QuestId> (&i->second), toUtf8Span (quest.getName()));
        }
    }

    void visitQuestName (QuestId questId, boost::function <void (Utf8Span)> visitor) const
    {
        MWDialogue::Quest const * quest = reinterpret_cast <MWDialogue::Quest const *> (questId);

        std::string name = quest->getName ();

        visitor (toUtf8Span (name));
    }

    template <typename iterator_t>
    struct JournalEntryImpl : BaseEntry <iterator_t, JournalEntry>
    {
        using BaseEntry <iterator_t, JournalEntry>::itr;

        mutable std::string timestamp_buffer;

        JournalEntryImpl (JournalViewModelImpl const * model, iterator_t itr) :
            BaseEntry <iterator_t, JournalEntry> (model, itr)
        {}

        std::string getText () const
        {
            return itr->getText();
        }

        Utf8Span timestamp () const
        {
            if (timestamp_buffer.empty ())
            {
                std::string dayStr = MyGUI::LanguageManager::getInstance().replaceTags("#{sDay}");

                std::ostringstream os;

                os
                    << itr->mDayOfMonth << ' '
                    << MWBase::Environment::get().getWorld()->getMonthName (itr->mMonth)
                    << " (" << dayStr << " " << (itr->mDay + 1) << ')';

                timestamp_buffer = os.str ();
            }

            return toUtf8Span (timestamp_buffer);
        }
    };

    void visitJournalEntries (QuestId questId, boost::function <void (JournalEntry const &)> visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        if (questId != 0)
        {
            MWDialogue::Quest const * quest = reinterpret_cast <MWDialogue::Quest const *> (questId);

            for(MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end (); ++i)
            {
                for (MWDialogue::Topic::TEntryIter j = quest->begin (); j != quest->end (); ++j)
                {
                    if (i->mInfoId == j->mInfoId)
                        visitor (JournalEntryImpl <MWBase::Journal::TEntryIter> (this, i));
                }
            }
        }
        else
        {
            for(MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end (); ++i)
                visitor (JournalEntryImpl <MWBase::Journal::TEntryIter> (this, i));
        }
    }

    void visitTopics (boost::function <void (TopicId, Utf8Span)> visitor) const
    {
        throw std::runtime_error ("not implemented");
    }

    void visitTopicName (TopicId topicId, boost::function <void (Utf8Span)> visitor) const
    {
        MWDialogue::Topic const & topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);
        visitor (toUtf8Span (topic.getName()));
    }

    void visitTopicNamesStartingWith (char character, boost::function < void (TopicId , Utf8Span) > visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            if (i->first [0] != std::tolower (character, mLocale))
                continue;

            visitor (TopicId (&i->second), toUtf8Span (i->second.getName()));
        }

    }

    struct TopicEntryImpl : BaseEntry <MWDialogue::Topic::TEntryIter, TopicEntry>
    {
        MWDialogue::Topic const & mTopic;

        TopicEntryImpl (JournalViewModelImpl const * model, MWDialogue::Topic const & topic, iterator_t itr) :
            BaseEntry (model, itr), mTopic (topic)
        {}

        std::string getText () const
        {
            return  itr->getText();
        }

        Utf8Span source () const
        {
            return toUtf8Span (itr->mActorName);
        }

    };

    void visitTopicEntries (TopicId topicId, boost::function <void (TopicEntry const &)> visitor) const
    {
        typedef MWDialogue::Topic::TEntryIter iterator_t;

        MWDialogue::Topic const & topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        for (iterator_t i = topic.begin (); i != topic.end (); ++i)
            visitor (TopicEntryImpl (this, topic, i));
    }
};

JournalViewModel::Ptr JournalViewModel::create ()
{
    return boost::make_shared <JournalViewModelImpl> ();
}

}
