#include "journalbooks.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/fontloader/fontloader.hpp>
#include <components/misc/utf8stream.hpp>

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

JournalBooks::JournalBooks (JournalViewModel::Ptr model, ToUTF8::FromType encoding) :
    mModel (model), mEncoding(encoding), mIndexPagesCount(0)
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
    bool isRussian = (mEncoding == ToUTF8::WINDOWS_1251);

    BookTypesetter::Ptr typesetter = isRussian ? createCyrillicJournalIndex() : createLatinJournalIndex();

    return typesetter->complete ();
}

BookTypesetter::Ptr JournalBooks::createLatinJournalIndex ()
{
    BookTypesetter::Ptr typesetter = BookTypesetter::create (92, 260);

    typesetter->setSectionAlignment (BookTypesetter::AlignCenter);

    // Latin journal index always has two columns for now.
    mIndexPagesCount = 2;

    char ch = 'A';

    BookTypesetter::Style* body = typesetter->createStyle ("", MyGUI::Colour::Black);
    for (int i = 0; i < 26; ++i)
    {
        char buffer [32];
        sprintf (buffer, "( %c )", ch);

        const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        BookTypesetter::Style* style = typesetter->createHotStyle (body, textColours.journalTopic,
                                                                   textColours.journalTopicOver,
                                                                   textColours.journalTopicPressed, (Utf8Stream::UnicodeChar) ch);

        if (i == 13)
            typesetter->sectionBreak ();

        typesetter->write (style, to_utf8_span (buffer));
        typesetter->lineBreak ();

        ch++;
    }

    return typesetter;
}

BookTypesetter::Ptr JournalBooks::createCyrillicJournalIndex ()
{
    BookTypesetter::Ptr typesetter = BookTypesetter::create (92, 260);

    typesetter->setSectionAlignment (BookTypesetter::AlignCenter);

    BookTypesetter::Style* body = typesetter->createStyle ("", MyGUI::Colour::Black);

    int fontHeight = MWBase::Environment::get().getWindowManager()->getFontHeight();

    // for small font size split alphabet to two columns (2x15 characers), for big font size split it to three colums (3x10 characters).
    int sectionBreak = 10;
    mIndexPagesCount = 3;
    if (fontHeight < 18)
    {
        sectionBreak = 15;
        mIndexPagesCount = 2;
    }

    unsigned char ch[2] = {0xd0, 0x90}; // CYRILLIC CAPITAL A is a 0xd090 in UTF-8

    for (int i = 0; i < 32; ++i)
    {
        char buffer [32];
        sprintf(buffer, "( %c%c )", ch[0], ch[1]);

        Utf8Stream stream ((char*) ch);
        Utf8Stream::UnicodeChar first = stream.peek();

        const MWGui::TextColours& textColours = MWBase::Environment::get().getWindowManager()->getTextColours();
        BookTypesetter::Style* style = typesetter->createHotStyle (body, textColours.journalTopic,
                                                                   textColours.journalTopicOver,
                                                                   textColours.journalTopicPressed, first);

        ch[1]++;

        // Words can not be started with these characters
        if (i == 26 || i == 28)
            continue;

        if (i % sectionBreak == 0)
            typesetter->sectionBreak ();

        typesetter->write (style, to_utf8_span (buffer));
        typesetter->lineBreak ();
    }

    return typesetter;
}

BookTypesetter::Ptr JournalBooks::createTypesetter ()
{
    //TODO: determine page size from layout...
    return BookTypesetter::create (240, 320);
}

}
