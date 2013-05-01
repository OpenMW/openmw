#include "journalviewmodel.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/environment.hpp"
#include "../mwdialogue/journalentry.hpp"

#include <MyGUI_LanguageManager.h>

#include <components/misc/utf8stream.hpp>

#include <map>
#include <sstream>
#include <boost/make_shared.hpp>

using namespace MWGui;

namespace MWGui { struct JournalViewModelImpl; }

static void injectMonthName (std::ostream & os, int month);

template <typename string_t, typename value_t>
class KeywordSearch
{
public:

    typedef typename string_t::const_iterator Point;

    struct Match
    {
        Point mBeg;
        Point mEnd;
        value_t mValue;
    };

    void seed (string_t keyword, value_t value)
    {
        seed_impl  (/*std::move*/ (keyword), /*std::move*/ (value), 0, mRoot);
    }

    void clear ()
    {
        mRoot.mChildren.clear ();
        mRoot.mKeyword.clear ();
    }

    bool search (Point beg, Point end, Match & match)
    {
        for (Point i = beg; i != end; ++i)
        {
            // check first character
            typename Entry::childen_t::iterator candidate = mRoot.mChildren.find (std::tolower (*i, mLocale));

            // no match, on to next character
            if (candidate == mRoot.mChildren.end ())
                continue;

            // see how far the match goes
            Point j = i;

            while ((j + 1) != end)
            {
                typename Entry::childen_t::iterator next = candidate->second.mChildren.find (std::tolower (*++j, mLocale));

                if (next == candidate->second.mChildren.end ())
                    break;

                candidate = next;
            }

            // didn't match enough to disambiguate, on to next character
            if (!candidate->second.mKeyword.size ())
                continue;

            // match the rest of the keyword
            typename string_t::const_iterator t = candidate->second.mKeyword.begin () + (j - i);

            while (j != end && t != candidate->second.mKeyword.end ())
            {
                if (std::tolower (*j, mLocale) != std::tolower (*t, mLocale))
                    break;

                ++j, ++t;
            }

            // didn't match full keyword, on to next character
            if (t != candidate->second.mKeyword.end ())
                continue;

            // we did it, report the good news
            match.mValue = candidate->second.mValue;
            match.mBeg = i;
            match.mEnd = j;

            return true;
        }

        // no match in range, report the bad news
        return false;
    }

private:

    struct Entry
    {
        typedef std::map <wchar_t, Entry> childen_t;

        string_t mKeyword;
        value_t mValue;
        childen_t mChildren;
    };

    void seed_impl (string_t keyword, value_t value, size_t depth, Entry  & entry)
    {
        int ch = tolower (keyword.at (depth), mLocale);

        typename Entry::childen_t::iterator j = entry.mChildren.find (ch);

        if (j == entry.mChildren.end ())
        {
            entry.mChildren [ch].mValue = /*std::move*/ (value);
            entry.mChildren [ch].mKeyword = /*std::move*/ (keyword);
        }
        else
        {
            if (j->second.mKeyword.size () > 0)
            {
                if (keyword == j->second.mKeyword)
                    throw std::runtime_error ("duplicate keyword inserted");

                value_t pushValue = /*std::move*/ (j->second.mValue);
                string_t pushKeyword = /*std::move*/ (j->second.mKeyword);

                j->second.mKeyword.clear ();

                if (depth >= pushKeyword.size ())
                    throw std::runtime_error ("unexpected");

                if (depth+1 < pushKeyword.size())
                    seed_impl (/*std::move*/ (pushKeyword), /*std::move*/ (pushValue), depth+1, j->second);
            }

            if (depth+1 < keyword.size())
                seed_impl (/*std::move*/ (keyword), /*std::move*/ (value), depth+1, j->second);
        }

    }

    Entry mRoot;
    std::locale mLocale;
};

struct MWGui::JournalViewModelImpl : JournalViewModel
{
    typedef KeywordSearch <std::string, intptr_t> keyword_search_t;

    mutable bool             mKeywordSearchLoaded;
    mutable keyword_search_t mKeywordSearch;

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

    template <typename t_iterator, typename IInterface>
    struct BaseEntry : IInterface
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

        virtual std::string getText () const = 0;

        void ensureLoaded () const
        {
            if (!loaded)
            {
                utf8text = getText ();
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

            std::string::const_iterator i = utf8text.begin ();

            keyword_search_t::Match match;

            while (i != utf8text.end () && mModel->mKeywordSearch.search (i, utf8text.end (), match))
            {
                if (i != match.mBeg)
                    visitor (0, i - utf8text.begin (), match.mBeg - utf8text.begin ());

                visitor (match.mValue, match.mBeg - utf8text.begin (), match.mEnd - utf8text.begin ());

                i = match.mEnd;
            }

            if (i != utf8text.end ())
                visitor (0, i - utf8text.begin (), utf8text.size ());
        }

    };

    void visitQuestNames (bool active_only, boost::function <void (QuestId, Utf8Span)> visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get ().getJournal ();

        for (MWBase::Journal::TQuestIter i = journal->questBegin (); i != journal->questEnd (); ++i)
        {
            if (active_only && i->second.isFinished ())
                continue;

            visitor (reinterpret_cast <QuestId> (&i->second), toUtf8Span (i->first));
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

        JournalEntryImpl (JournalViewModelImpl const * Model, iterator_t itr) :
            BaseEntry <iterator_t, JournalEntry> (Model, itr)
        {}

        std::string getText () const
        {
            return itr->getText(MWBase::Environment::get().getWorld()->getStore());
        }

        Utf8Span timestamp () const
        {
            if (timestamp_buffer.empty ())
            {
                std::ostringstream os;

                os << itr->mDayOfMonth << ' ';

                injectMonthName (os, itr->mMonth);

                const std::string& dayStr = MyGUI::LanguageManager::getInstance().replaceTags("#{sDay}");
                os << " (" << dayStr << " " << (itr->mDay + 1) << ')';

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
                    if (i->mInfoId == *j)
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

        visitor (toUtf8Span (topic.getName ()));
    }

    void visitTopicNamesStartingWith (char character, boost::function < void (TopicId , Utf8Span) > visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            if (i->first [0] != std::tolower (character, mLocale))
                continue;

            visitor (TopicId (&i->second), toUtf8Span (i->first));
        }

    }

    struct TopicEntryImpl : BaseEntry <MWDialogue::Topic::TEntryIter, TopicEntry>
    {
        MWDialogue::Topic const & mTopic;

        mutable std::string source_buffer;

        TopicEntryImpl (JournalViewModelImpl const * model, MWDialogue::Topic const & topic, iterator_t itr) :
            BaseEntry (model, itr), mTopic (topic)
        {}

        std::string getText () const
        {
            /// \todo defines are not replaced (%PCName etc). should probably be done elsewhere though since we need the actor
            return  mTopic.getEntry (*itr).getText(MWBase::Environment::get().getWorld()->getStore());

        }

        Utf8Span source () const
        {
            if (source_buffer.empty ())
                source_buffer = "someone";
            return toUtf8Span (source_buffer);
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

static void injectMonthName (std::ostream & os, int month)
{
    MyGUI::LanguageManager& lm = MyGUI::LanguageManager::getInstance();

    if (month  == 0)
        os << lm.replaceTags ("#{sMonthMorningstar}");
    else if (month == 1)
        os << lm.replaceTags ("#{sMonthSunsdawn}");
    else if (month == 2)
        os << lm.replaceTags ("#{sMonthFirstseed}");
    else if (month == 3)
        os << lm.replaceTags ("#{sMonthRainshand}");
    else if (month == 4)
        os << lm.replaceTags ("#{sMonthSecondseed}");
    else if (month == 5)
        os << lm.replaceTags ("#{sMonthMidyear}");
    else if (month == 6)
        os << lm.replaceTags ("#{sMonthSunsheight}");
    else if (month == 7)
        os << lm.replaceTags ("#{sMonthLastseed}");
    else if (month == 8)
        os << lm.replaceTags ("#{sMonthHeartfire}");
    else if (month == 9)
        os << lm.replaceTags ("#{sMonthFrostfall}");
    else if (month == 10)
        os << lm.replaceTags ("#{sMonthSunsdusk}");
    else if (month == 11)
        os << lm.replaceTags ("#{sMonthEveningstar}");
    else
        os << month;
}

JournalViewModel::Ptr JournalViewModel::create ()
{
    return boost::make_shared <JournalViewModelImpl> ();
}
