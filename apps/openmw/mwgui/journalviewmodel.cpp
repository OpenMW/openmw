#include "journalviewmodel.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/environment.hpp"
#include "../mwdialogue/journalentry.hpp"

//#include "MyGUI_LanguageManager.h"

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

    typedef typename string_t::const_iterator point;

    struct Match
    {
        point mBeg;
        point mEnd;
        value_t mValue;
    };

    void seed (string_t Keyword, value_t Value)
    {
        seed_impl  (/*std::move*/ (Keyword), /*std::move*/ (Value), 0, mRoot);
    }

    void clear ()
    {
        mRoot.mChildren.clear ();
        mRoot.mKeyword.clear ();
    }

    bool search (point Beg, point End, Match & Match)
    {
        for (point i = Beg; i != End; ++i)
        {
            // check first character
            typename Entry::childen_t::iterator candidate = mRoot.mChildren.find (std::tolower (*i, mLocale));

            // no match, on to next character
            if (candidate == mRoot.mChildren.end ())
                continue;

            // see how far the match goes
            point j = i;

            while ((j + 1) != End)
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

            while (j != End && t != candidate->second.mKeyword.end ())
            {
                if (std::tolower (*j, mLocale) != std::tolower (*t, mLocale))
                    break;

                ++j, ++t;
            }

            // didn't match full keyword, on to next character
            if (t != candidate->second.mKeyword.end ())
                continue;

            // we did it, report the good news
            Match.mValue = candidate->second.mValue;
            Match.mBeg = i;
            Match.mEnd = j;

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

    void seed_impl (string_t Keyword, value_t Value, size_t Depth, Entry  & Entry)
    {
        int ch = tolower (Keyword.at (Depth), mLocale);

        typename Entry::childen_t::iterator j = Entry.mChildren.find (ch);

        if (j == Entry.mChildren.end ())
        {
            Entry.mChildren [ch].mValue = /*std::move*/ (Value);
            Entry.mChildren [ch].mKeyword = /*std::move*/ (Keyword);
        }
        else
        {
            if (j->second.mKeyword.size () > 0)
            {
                if (Keyword == j->second.mKeyword)
                    throw std::runtime_error ("duplicate keyword inserted");

                value_t pushValue = /*std::move*/ (j->second.mValue);
                string_t pushKeyword = /*std::move*/ (j->second.mKeyword);

                j->second.mKeyword.clear ();

                if (Depth >= pushKeyword.size ())
                    throw std::runtime_error ("unexpected");

                seed_impl (/*std::move*/ (pushKeyword), /*std::move*/ (pushValue), Depth+1, j->second);
            }

            seed_impl (/*std::move*/ (Keyword), /*std::move*/ (Value), Depth+1, j->second);
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

    //TODO: replace this nasty BS
    static utf8_span toUtf8Span (std::string const & str)
    {
        if (str.size () == 0)
            return utf8_span (utf8_point (NULL), utf8_point (NULL));

        utf8_point point = reinterpret_cast <utf8_point> (str.c_str ());

        return utf8_span (point, point + str.size ());
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
        JournalViewModelImpl const *    Model;

        BaseEntry (JournalViewModelImpl const * Model, iterator_t itr) :
            Model (Model), itr (itr), loaded (false)
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

        utf8_span body () const
        {
            ensureLoaded ();

            return toUtf8Span (utf8text);
        }

        void visitSpans (boost::function < void (topic_id, size_t, size_t)> visitor) const
        {
            ensureLoaded ();
            Model->ensureKeyWordSearchLoaded ();

            std::string::const_iterator i = utf8text.begin ();

            keyword_search_t::Match match;

            while (i != utf8text.end () && Model->mKeywordSearch.search (i, utf8text.end (), match))
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

    void visitQuestNames (bool active_only, boost::function <void (quest_id, utf8_span)> visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get ().getJournal ();

        for (MWBase::Journal::TQuestIter i = journal->questBegin (); i != journal->questEnd (); ++i)
        {
            if (active_only && i->second.isFinished ())
                continue;

            visitor (reinterpret_cast <quest_id> (&i->second), toUtf8Span (i->first));
        }
    }

    void visitQuestName (quest_id questId, boost::function <void (utf8_span)> visitor) const
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

        utf8_span timestamp () const
        {
            if (timestamp_buffer.empty ())
            {
                std::ostringstream os;

                os << itr->mDayOfMonth << ' ';

                injectMonthName (os, itr->mMonth);

                os << " (Day " << (itr->mDay + 1) << ')';

                timestamp_buffer = os.str ();
            }

            return toUtf8Span (timestamp_buffer);
        }
    };

    void visitJournalEntries (quest_id questId, boost::function <void (JournalEntry const &)> visitor) const
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

    void visitTopics (boost::function <void (topic_id, utf8_span)> visitor) const
    {
        throw std::runtime_error ("not implemented");
    }

    void visitTopicName (topic_id topicId, boost::function <void (utf8_span)> visitor) const
    {
        MWDialogue::Topic const & Topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        visitor (toUtf8Span (Topic.getName ()));
    }

    void visitTopicNamesStartingWith (int character, boost::function < void (topic_id , utf8_span) > visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            if (i->first [0] != std::tolower (character, mLocale))
                continue;

            visitor (topic_id (&i->second), toUtf8Span (i->first));
        }

    }

    struct TopicEntryImpl : BaseEntry <MWDialogue::Topic::TEntryIter, TopicEntry>
    {
        MWDialogue::Topic const & Topic;

        mutable std::string source_buffer;

        TopicEntryImpl (JournalViewModelImpl const * Model, MWDialogue::Topic const & Topic, iterator_t itr) :
            BaseEntry (Model, itr), Topic (Topic)
        {}

        std::string getText () const
        {
            return Topic.getEntry (*itr).getText(MWBase::Environment::get().getWorld()->getStore());

        }

        utf8_span source () const
        {
            if (source_buffer.empty ())
                source_buffer = "someone";
            return toUtf8Span (source_buffer);
        }

    };

    void visitTopicEntries (topic_id topicId, boost::function <void (TopicEntry const &)> visitor) const
    {
        typedef MWDialogue::Topic::TEntryIter iterator_t;

        MWDialogue::Topic const & Topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        for (iterator_t i = Topic.begin (); i != Topic.end (); ++i)
            visitor (TopicEntryImpl (this, Topic, i));
    }
};

static void injectMonthName (std::ostream & os, int month)
{
#ifndef MYGUI_SIGNLETON_FIXED_FOR_MSVC
    static char const * month_names [] =
    {
        "Morning Star",
        "Sun's Dawn",
        "First Seed",
        "Rain's Hand",
        "Second Seed",
        "Midyear",
        "Sun's Height",
        "Last Seed",
        "Hearthfire",
        "Frostfall",
        "Sun's Dusk",
        "Evening Star",
    };
    if (month >= 0 && month  <= 11)
        os << month_names [month ];
    else
        os << month ;
#else
    auto lm = MyGUI::LanguageManager::getInstance();

    if (month  == 0)
        os << lm.getTag ("sMonthMorningstar");
    else if (month == 1)
        os << lm.getTag ("sMonthSunsdawn");
    else if (month == 2)
        os << lm.getTag ("sMonthFirstseed");
    else if (month == 3)
        os << lm.getTag ("sMonthRainshand");
    else if (month == 4)
        os << lm.getTag ("sMonthSecondseed");
    else if (month == 5)
        os << lm.getTag ("sMonthMidyear");
    else if (month == 6)
        os << lm.getTag ("sMonthSunsheight");
    else if (month == 7)
        os << lm.getTag ("sMonthLastseed");
    else if (month == 8)
        os << lm.getTag ("sMonthHeartfire");
    else if (month == 9)
        os << lm.getTag ("sMonthFrostfall");
    else if (month == 10)
        os << lm.getTag ("sMonthSunsdusk");
    else if (month == 11)
        os << lm.getTag ("sMonthEveningstar");
    else
        os << month;
#endif
}

JournalViewModel::ptr JournalViewModel::create ()
{
    return boost::make_shared <JournalViewModelImpl> ();
}
