#include "journalbooks.hpp"

namespace
{
    MWGui::BookTypesetter::Utf8Span to_utf8_span (char const * text)
    {
        typedef MWGui::BookTypesetter::Utf8Point point;

        point begin = reinterpret_cast <point> (text);

        return MWGui::BookTypesetter::Utf8Span (begin, begin + strlen (text));
    }

    const MyGUI::Colour linkHot    (0.40f, 0.40f, 0.80f);
    const MyGUI::Colour linkNormal (0.20f, 0.20f, 0.60f);
    const MyGUI::Colour linkActive (0.50f, 0.50f, 1.00f);

    struct AddContent
    {
        MWGui::BookTypesetter::Ptr mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        AddContent (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style) :
            mTypesetter (typesetter), mBodyStyle (body_style)
        {
        }
    };

    struct AddSpan : AddContent
    {
        AddSpan (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style) :
            AddContent (typesetter, body_style)
        {
        }

        void operator () (intptr_t topicId, size_t begin, size_t end)
        {
            MWGui::BookTypesetter::Style* style = mBodyStyle;

            if (topicId)
                style = mTypesetter->createHotStyle (mBodyStyle, linkNormal, linkHot, linkActive, topicId);

            mTypesetter->write (style, begin, end);
        }
    };

    struct AddEntry
    {
        MWGui::BookTypesetter::Ptr mTypesetter;
        MWGui::BookTypesetter::Style* mBodyStyle;

        AddEntry (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style) :
            mTypesetter (typesetter), mBodyStyle (body_style)
        {
        }

        void operator () (MWGui::JournalViewModel::Entry const & entry)
        {
            mTypesetter->addContent (entry.body ());

            entry.visitSpans (AddSpan (mTypesetter, mBodyStyle));
        }
    };

    struct AddJournalEntry : AddEntry
    {
        bool mAddHeader;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        AddJournalEntry (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style,
                            MWGui::BookTypesetter::Style* header_style, bool add_header) :
            AddEntry (typesetter, body_style),
            mHeaderStyle (header_style),
            mAddHeader (add_header)
        {
        }

        void operator () (MWGui::JournalViewModel::JournalEntry const & entry)
        {
            if (mAddHeader)
            {
                mTypesetter->write (mHeaderStyle, entry.timestamp ());
                mTypesetter->lineBreak ();
            }

            AddEntry::operator () (entry);

            mTypesetter->sectionBreak (10);
        }
    };

    struct AddTopicEntry : AddEntry
    {
        intptr_t mContentId;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        AddTopicEntry (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style,
                        MWGui::BookTypesetter::Style* header_style, intptr_t contentId) :
            AddEntry (typesetter, body_style), mHeaderStyle (header_style), mContentId (contentId)
        {
        }

        void operator () (MWGui::JournalViewModel::TopicEntry const & entry)
        {
            mTypesetter->write (mBodyStyle, entry.source ());
            mTypesetter->write (mBodyStyle, 0, 3);// begin

            AddEntry::operator() (entry);

            mTypesetter->selectContent (mContentId);
            mTypesetter->write (mBodyStyle, 2, 3);// end quote

            mTypesetter->sectionBreak (10);
        }
    };

    struct AddTopicName : AddContent
    {
        AddTopicName (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* style) :
            AddContent (typesetter, style)
        {
        }

        void operator () (MWGui::JournalViewModel::Utf8Span topicName)
        {
            mTypesetter->write (mBodyStyle, topicName);
            mTypesetter->sectionBreak (10);
        }
    };

    struct AddQuestName : AddContent
    {
        AddQuestName (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* style) :
            AddContent (typesetter, style)
        {
        }

        void operator () (MWGui::JournalViewModel::Utf8Span topicName)
        {
            mTypesetter->write (mBodyStyle, topicName);
            mTypesetter->sectionBreak (10);
        }
    };

    struct AddTopicLink : AddContent
    {
        AddTopicLink (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* style) :
            AddContent (typesetter, style)
        {
        }

        void operator () (MWGui::JournalViewModel::TopicId topicId, MWGui::JournalViewModel::Utf8Span name)
        {
            MWGui::BookTypesetter::Style* link = mTypesetter->createHotStyle (mBodyStyle, MyGUI::Colour::Black, linkHot, linkActive, topicId);

            mTypesetter->write (link, name);
            mTypesetter->lineBreak ();
        }
    };

    struct AddQuestLink : AddContent
    {
        AddQuestLink (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* style) :
            AddContent (typesetter, style)
        {
        }

        void operator () (MWGui::JournalViewModel::QuestId id, MWGui::JournalViewModel::Utf8Span name)
        {
            MWGui::BookTypesetter::Style* style = mTypesetter->createHotStyle (mBodyStyle, MyGUI::Colour::Black, linkHot, linkActive, id);

            mTypesetter->write (style, name);
            mTypesetter->lineBreak ();
        }
    };
}

namespace MWGui
{

typedef TypesetBook::Ptr book;

JournalBooks::JournalBooks (JournalViewModel::Ptr model) :
    mModel (model)
{
}

book JournalBooks::createEmptyJournalBook ()
{
    BookTypesetter::Ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    typesetter->write (header, to_utf8_span ("You have no journal entries!"));
    typesetter->lineBreak ();
    typesetter->write (body, to_utf8_span ("You should have gone though the starting quest and got an initial quest."));

    return typesetter->complete ();
}

book JournalBooks::createJournalBook ()
{
    BookTypesetter::Ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    mModel->visitJournalEntries (0, AddJournalEntry (typesetter, body, header, true));

    return typesetter->complete ();
}

book JournalBooks::createTopicBook (uintptr_t topicId)
{
    BookTypesetter::Ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    mModel->visitTopicName (topicId, AddTopicName (typesetter, header));

    intptr_t contentId = typesetter->addContent (to_utf8_span (", \""));

    mModel->visitTopicEntries (topicId, AddTopicEntry (typesetter, body, header, contentId));

    return typesetter->complete ();
}

book JournalBooks::createQuestBook (uintptr_t questId)
{
    BookTypesetter::Ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    mModel->visitQuestName (questId, AddQuestName (typesetter, header));

    mModel->visitJournalEntries (questId, AddJournalEntry (typesetter, body, header, false));

    return typesetter->complete ();
}

book JournalBooks::createTopicIndexBook ()
{
    BookTypesetter::Ptr typesetter = BookTypesetter::create (92, 250);

    typesetter->setSectionAlignment (BookTypesetter::AlignCenter);

    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    for (int i = 0; i < 26; ++i)
    {
        char ch = 'A' + i;

        char buffer [32];

        sprintf (buffer, "( %c )", ch);

        BookTypesetter::Style* style = typesetter->createHotStyle (body, MyGUI::Colour::Black, linkHot, linkActive, ch);

        if (i == 13)
            typesetter->sectionBreak ();

        typesetter->write (style, to_utf8_span (buffer));
        typesetter->lineBreak ();
    }

    return typesetter->complete ();
}

book JournalBooks::createTopicIndexBook (char character)
{
    BookTypesetter::Ptr typesetter = BookTypesetter::create (0x7FFFFFFF, 0x7FFFFFFF);
    BookTypesetter::Style* style = typesetter->createStyle ("", MyGUI::Colour::Black);

    mModel->visitTopicNamesStartingWith (character, AddTopicLink (typesetter, style));

    return typesetter->complete ();
}

book JournalBooks::createQuestIndexBook (bool activeOnly)
{
    BookTypesetter::Ptr typesetter = BookTypesetter::create (0x7FFFFFFF, 0x7FFFFFFF);
    BookTypesetter::Style* base = typesetter->createStyle ("", MyGUI::Colour::Black);

    mModel->visitQuestNames (activeOnly, AddQuestLink (typesetter, base));

    return typesetter->complete ();
}

BookTypesetter::Ptr JournalBooks::createTypesetter ()
{
    //TODO: determine page size from layout...
    return BookTypesetter::create (240, 300);
}

}
