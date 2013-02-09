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

namespace MWGui { struct JournalViewModel; }

static void injectMonthName (std::ostream & os, int month);

template <typename string_t, typename value_t>
class keyword_search
{
public:

    typedef typename string_t::const_iterator point;

    struct match
    {
        point Beg;
        point End;
        value_t Value;
    };

    void seed (string_t Keyword, value_t Value)
    {
        seed_impl  (/*std::move*/ (Keyword), /*std::move*/ (Value), 0, Root);
    }

    void clear ()
    {
        Root.Children.clear ();
        Root.Keyword.clear ();
    }

    bool search (point Beg, point End, match & Match)
    {
        for (point i = Beg; i != End; ++i)
        {
            // check first character
            typename entry::childen_t::iterator candidate = Root.Children.find (std::tolower (*i, Locale));

            // no match, on to next character
            if (candidate == Root.Children.end ())
                continue;

            // see how far the match goes
            point j = i;

            while ((j + 1) != End)
            {
                typename entry::childen_t::iterator next = candidate->second.Children.find (std::tolower (*++j, Locale));

                if (next == candidate->second.Children.end ())
                    break;

                candidate = next;
            }

            // didn't match enough to disambiguate, on to next character
            if (!candidate->second.Keyword.size ())
                continue;

            // match the rest of the keyword
            typename string_t::const_iterator t = candidate->second.Keyword.begin () + (j - i);

            while (j != End && t != candidate->second.Keyword.end ())
            {
                if (std::tolower (*j, Locale) != std::tolower (*t, Locale))
                    break;

                ++j, ++t;
            }

            // didn't match full keyword, on to next character
            if (t != candidate->second.Keyword.end ())
                continue;

            // we did it, report the good news
            Match.Value = candidate->second.Value;
            Match.Beg = i;
            Match.End = j;

            return true;
        }

        // no match in range, report the bad news
        return false;
    }

private:

    struct entry
    {
        typedef std::map <wchar_t, entry> childen_t;

        string_t Keyword;
        value_t Value;
        childen_t Children;
    };

    void seed_impl (string_t Keyword, value_t Value, size_t Depth, entry  & Entry)
    {
        int ch = tolower (Keyword.at (Depth), Locale);

        typename entry::childen_t::iterator j = Entry.Children.find (ch);

        if (j == Entry.Children.end ())
        {
            Entry.Children [ch].Value = /*std::move*/ (Value);
            Entry.Children [ch].Keyword = /*std::move*/ (Keyword);
        }
        else
        {
            if (j->second.Keyword.size () > 0)
            {
                if (Keyword == j->second.Keyword)
                    throw std::runtime_error ("duplicate keyword inserted");

                value_t pushValue = /*std::move*/ (j->second.Value);
                string_t pushKeyword = /*std::move*/ (j->second.Keyword);

                j->second.Keyword.clear ();

                if (Depth >= pushKeyword.size ())
                    throw std::runtime_error ("unexpected");

                seed_impl (/*std::move*/ (pushKeyword), /*std::move*/ (pushValue), Depth+1, j->second);
            }

            seed_impl (/*std::move*/ (Keyword), /*std::move*/ (Value), Depth+1, j->second);
        }

    }

    entry Root;
    std::locale Locale;
};

struct MWGui::JournalViewModel : IJournalViewModel
{
    typedef keyword_search <std::string, intptr_t> foobar;

    mutable bool FooBar_loaded;
    mutable foobar FooBar;

    std::locale Locale;

    JournalViewModel ()
    {
        FooBar_loaded = false;
    }

    virtual ~JournalViewModel ()
    {
    }

    //TODO: replace this nasty BS
    static utf8_span to_utf8_span (std::string const & str)
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
        FooBar.clear ();
        FooBar_loaded = false;
    }

    void ensure_FooBar_loaded () const
    {
        if (!FooBar_loaded)
        {
            MWBase::Journal * journal = MWBase::Environment::get().getJournal();

            for(MWBase::Journal::TTopicIter i = journal->topicBegin(); i != journal->topicEnd (); ++i)
                FooBar.seed (i->first, intptr_t (&i->second));

            FooBar_loaded = true;
        }
    }

    wchar_t tolower (wchar_t ch) const { return std::tolower (ch, Locale); }

    bool is_empty () const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        return journal->begin () == journal->end ();
    }

    template <typename t_iterator, typename IInterface>
    struct base_entry : IInterface
    {
        typedef t_iterator iterator_t;

        iterator_t                  itr;
        JournalViewModel const *    Model;

        base_entry (JournalViewModel const * Model, iterator_t itr) :
            Model (Model), itr (itr), loaded (false)
        {}

        virtual ~base_entry () {}

        mutable bool loaded;
        mutable std::string utf8text;

        virtual std::string getText () const = 0;

        void ensure_loaded () const
        {
            if (!loaded)
            {
                utf8text = getText ();
                loaded = true;
            }
        }

        utf8_span body () const
        {
            ensure_loaded ();

            return to_utf8_span (utf8text);
        }

        void visit_spans (boost::function < void (topic_id, size_t, size_t)> visitor) const
        {
            ensure_loaded ();
            Model->ensure_FooBar_loaded ();

            std::string::const_iterator i = utf8text.begin ();

            foobar::match Match;

            while (i != utf8text.end () && Model->FooBar.search (i, utf8text.end (), Match))
            {
                if (i != Match.Beg)
                    visitor (0, i - utf8text.begin (), Match.Beg - utf8text.begin ());

                visitor (Match.Value, Match.Beg - utf8text.begin (), Match.End - utf8text.begin ());

                i = Match.End;
            }

            if (i != utf8text.end ())
                visitor (0, i - utf8text.begin (), utf8text.size ());
        }

    };

    void visit_quest_names (bool active_only, boost::function <void (quest_id, utf8_span)> visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get ().getJournal ();

        for (MWBase::Journal::TQuestIter i = journal->questBegin (); i != journal->questEnd (); ++i)
        {
            if (active_only && i->second.isFinished ())
                continue;

            visitor (reinterpret_cast <quest_id> (&i->second), to_utf8_span (i->first));
        }
    }

    void visit_quest_name (quest_id questId, boost::function <void (utf8_span)> visitor) const
    {
        MWDialogue::Quest const * quest = reinterpret_cast <MWDialogue::Quest const *> (questId);

        std::string name = quest->getName ();

        visitor (to_utf8_span (name));
    }

    template <typename iterator_t>
    struct journal_entry : base_entry <iterator_t, IJournalEntry>
    {
        using base_entry <iterator_t, IJournalEntry>::itr;

        mutable std::string timestamp_buffer;

        journal_entry (JournalViewModel const * Model, iterator_t itr) :
            base_entry <iterator_t, IJournalEntry> (Model, itr)
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

            return to_utf8_span (timestamp_buffer);
        }
    };

    void visit_journal_entries (quest_id questId, boost::function <void (IJournalEntry const &)> visitor) const
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
                        visitor (journal_entry <MWBase::Journal::TEntryIter> (this, i));
                }
            }
        }
        else
        {
            for(MWBase::Journal::TEntryIter i = journal->begin(); i != journal->end (); ++i)
                visitor (journal_entry <MWBase::Journal::TEntryIter> (this, i));
        }
    }

    void visit_topics (boost::function <void (topic_id, utf8_span)> visitor) const
    {
        throw std::runtime_error ("not implemented");
    }

    void visit_topic_name (topic_id topicId, boost::function <void (utf8_span)> visitor) const
    {
        MWDialogue::Topic const & Topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        visitor (to_utf8_span (Topic.getName ()));
    }

    void visit_topic_names_starting_with (int character, boost::function < void (topic_id , utf8_span) > visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            if (i->first [0] != std::tolower (character, Locale))
                continue;

            visitor (topic_id (&i->second), to_utf8_span (i->first));
        }

    }

    struct topicEntry : base_entry <MWDialogue::Topic::TEntryIter, ITopicEntry>
    {
        MWDialogue::Topic const & Topic;

        mutable std::string source_buffer;

        topicEntry (JournalViewModel const * Model, MWDialogue::Topic const & Topic, iterator_t itr) :
            base_entry (Model, itr), Topic (Topic)
        {}

        std::string getText () const
        {
            return Topic.getEntry (*itr).getText(MWBase::Environment::get().getWorld()->getStore());

        }

        utf8_span source () const
        {
            if (source_buffer.empty ())
                source_buffer = "someone";
            return to_utf8_span (source_buffer);
        }

    };

    void visit_topic_entries (topic_id topicId, boost::function <void (ITopicEntry const &)> visitor) const
    {
        typedef MWDialogue::Topic::TEntryIter iterator_t;
		
        MWDialogue::Topic const & Topic = * reinterpret_cast <MWDialogue::Topic const *> (topicId);

        for (iterator_t i = Topic.begin (); i != Topic.end (); ++i)
            visitor (topicEntry (this, Topic, i));
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

IJournalViewModel::ptr IJournalViewModel::create ()
{
    return boost::make_shared <JournalViewModel> ();
}
