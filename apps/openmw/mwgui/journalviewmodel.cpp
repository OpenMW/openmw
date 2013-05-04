#include "journalviewmodel.hpp"

#include <map>
#include <sstream>
#include <boost/make_shared.hpp>

#include <MyGUI_LanguageManager.h>

#include <components/misc/utf8stream.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/environment.hpp"
#include "../mwdialogue/journalentry.hpp"

#include "keywordsearch.hpp"


using namespace MWGui;

namespace MWGui { struct JournalViewModelImpl; }

static void injectMonthName (std::ostream & os, int month);

struct MWGui::JournalViewModelImpl : JournalViewModel
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

            KeywordSearchT::Match match;

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

            /// \todo quest.getName() is broken? returns empty string
            //const MWDialogue::Quest& quest = i->second;

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

        JournalEntryImpl (JournalViewModelImpl const * model, iterator_t itr) :
            BaseEntry <iterator_t, JournalEntry> (model, itr)
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
        // This is to get the correct case for the topic
        const std::string& name = MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find(topic.getName())->mId;
        visitor (toUtf8Span (name));
    }

    void visitTopicNamesStartingWith (char character, boost::function < void (TopicId , Utf8Span) > visitor) const
    {
        MWBase::Journal * journal = MWBase::Environment::get().getJournal();

        for (MWBase::Journal::TTopicIter i = journal->topicBegin (); i != journal->topicEnd (); ++i)
        {
            if (i->first [0] != std::tolower (character, mLocale))
                continue;

            // This is to get the correct case for the topic
            const std::string& name = MWBase::Environment::get().getWorld()->getStore().get<ESM::Dialogue>().find(i->first)->mId;

            visitor (TopicId (&i->second), toUtf8Span (name));
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
