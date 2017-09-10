#include "journalbooks.hpp"

#include <MyGUI_LanguageManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "textcolours.hpp"


namespace
{
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

            const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
            if (topicId)
                style = mTypesetter->createHotStyle (mBodyStyle, textColours.journalLink, textColours.journalLinkOver,
                                                     textColours.journalLinkPressed, topicId);

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
            mAddHeader (add_header),
            mHeaderStyle (header_style)
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

            mTypesetter->sectionBreak (30);
        }
    };

    struct AddTopicEntry : AddEntry
    {
        intptr_t mContentId;
        MWGui::BookTypesetter::Style* mHeaderStyle;

        AddTopicEntry (MWGui::BookTypesetter::Ptr typesetter, MWGui::BookTypesetter::Style* body_style,
                        MWGui::BookTypesetter::Style* header_style, intptr_t contentId) :
            AddEntry (typesetter, body_style), mContentId (contentId), mHeaderStyle (header_style)
        {
        }

        void operator () (MWGui::JournalViewModel::TopicEntry const & entry)
        {
            mTypesetter->write (mBodyStyle, entry.source ());
            mTypesetter->write (mBodyStyle, 0, 3);// begin

            AddEntry::operator() (entry);

            mTypesetter->selectContent (mContentId);
            mTypesetter->write (mBodyStyle, 2, 3);// end quote

            mTypesetter->sectionBreak (30);
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
            mTypesetter->sectionBreak ();
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
            mTypesetter->sectionBreak ();
        }
    };
}

namespace MWGui
{

MWGui::BookTypesetter::Utf8Span to_utf8_span (char const * text)
{
    typedef MWGui::BookTypesetter::Utf8Point point;

    point begin = reinterpret_cast <point> (text);

    return MWGui::BookTypesetter::Utf8Span (begin, begin + strlen (text));
}

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

    mModel->visitJournalEntries ("", AddJournalEntry (typesetter, body, header, true));

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

book JournalBooks::createQuestBook (const std::string& questName)
{
    BookTypesetter::Ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("", MyGUI::Colour::Black);

    AddQuestName addName (typesetter, header);
    addName(to_utf8_span(questName.c_str()));

    mModel->visitJournalEntries (questName, AddJournalEntry (typesetter, body, header, false));

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

        const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        BookTypesetter::Style* style = typesetter->createHotStyle (body, textColours.journalTopic,
                                                                   textColours.journalTopicOver,
                                                                   textColours.journalTopicPressed, ch);

        if (i == 13)
            typesetter->sectionBreak ();

        typesetter->write (style, to_utf8_span (buffer));
        typesetter->lineBreak ();
    }

    return typesetter->complete ();
}

BookTypesetter::Ptr JournalBooks::createTypesetter ()
{
    //TODO: determine page size from layout...
    return BookTypesetter::create (240, 320);
}

}
