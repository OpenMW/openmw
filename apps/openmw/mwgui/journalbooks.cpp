#include "journalbooks.hpp"

using namespace MWGui;

namespace
{
    BookTypesetter::utf8_span to_utf8_span (char const * Text)
    {
        typedef BookTypesetter::utf8_point point;

        point begin = reinterpret_cast <point> (Text);

        return BookTypesetter::utf8_span (begin, begin + strlen (Text));
    }

    const MyGUI::Colour linkHot    (0.40f, 0.40f, 0.80f);
    const MyGUI::Colour linkNormal (0.20f, 0.20f, 0.60f);
    const MyGUI::Colour linkActive (0.50f, 0.50f, 1.00f);

    struct addContent
    {
        BookTypesetter::ptr typesetter;
        BookTypesetter::Style* body_style;

        addContent (BookTypesetter::ptr typesetter, BookTypesetter::Style* body_style) :
            typesetter (typesetter), body_style (body_style)
        {
        }
    };

    struct addSpan : addContent
    {
        addSpan (BookTypesetter::ptr typesetter, BookTypesetter::Style* body_style) :
            addContent (typesetter, body_style)
        {
        }

        void operator () (intptr_t topicId, size_t begin, size_t end)
        {
            BookTypesetter::Style* style = body_style;

            if (topicId)
                style = typesetter->createHotStyle (body_style, linkNormal, linkHot, linkActive, topicId);

            typesetter->write (style, begin, end);
        }
    };

    struct addEntry
    {
        BookTypesetter::ptr typesetter;
        BookTypesetter::Style* body_style;

        addEntry (BookTypesetter::ptr typesetter, BookTypesetter::Style* body_style) :
            typesetter (typesetter), body_style (body_style)
        {
        }

        void operator () (IJournalViewModel::IEntry const & Entry)
        {
            typesetter->add_content (Entry.body ());

            Entry.visitSpans (addSpan (typesetter, body_style));
        }
    };

    struct addJournalEntry : addEntry
    {
        bool add_header;
        BookTypesetter::Style* header_style;

        addJournalEntry (BookTypesetter::ptr typesetter, BookTypesetter::Style* body_style,
                            BookTypesetter::Style* header_style, bool add_header) :
            addEntry (typesetter, body_style),
            header_style (header_style),
            add_header (add_header)
        {
        }

        void operator () (IJournalViewModel::IJournalEntry const & Entry)
        {
            if (add_header)
            {
                typesetter->write (header_style, Entry.timestamp ());
                typesetter->lineBreak ();
            }

            addEntry::operator () (Entry);

            typesetter->sectionBreak (10);
        }
    };

    struct addTopicEntry : addEntry
    {
        intptr_t contentId;
        BookTypesetter::Style* header_style;

        addTopicEntry (BookTypesetter::ptr typesetter, BookTypesetter::Style* body_style,
                        BookTypesetter::Style* header_style, intptr_t contentId) :
            addEntry (typesetter, body_style), header_style (header_style), contentId (contentId)
        {
        }

        void operator () (IJournalViewModel::ITopicEntry const & Entry)
        {
            typesetter->write (body_style, Entry.source ());
            typesetter->write (body_style, 0, 3);// begin

            addEntry::operator() (Entry);

            typesetter->select_content (contentId);
            typesetter->write (body_style, 2, 3);// end quote

            typesetter->sectionBreak (10);
        }
    };

    struct addTopicName : addContent
    {
        addTopicName (BookTypesetter::ptr typesetter, BookTypesetter::Style* style) :
            addContent (typesetter, style)
        {
        }

        void operator () (IJournalViewModel::utf8_span topicName)
        {
            typesetter->write (body_style, topicName);
            typesetter->sectionBreak (10);
        }
    };

    struct addQuestName : addContent
    {
        addQuestName (BookTypesetter::ptr typesetter, BookTypesetter::Style* style) :
            addContent (typesetter, style)
        {
        }

        void operator () (IJournalViewModel::utf8_span topicName)
        {
            typesetter->write (body_style, topicName);
            typesetter->sectionBreak (10);
        }
    };

    struct addTopicLink : addContent
    {
        addTopicLink (BookTypesetter::ptr typesetter, BookTypesetter::Style* style) :
            addContent (typesetter, style)
        {
        }

        void operator () (IJournalViewModel::topic_id topicId, IJournalViewModel::utf8_span name)
        {
            BookTypesetter::Style* link = typesetter->createHotStyle (body_style, MyGUI::Colour::Black, linkHot, linkActive, topicId);

            typesetter->write (link, name);
            typesetter->lineBreak ();
        }
    };

    struct addQuestLink : addContent
    {
        addQuestLink (BookTypesetter::ptr typesetter, BookTypesetter::Style* style) :
            addContent (typesetter, style)
        {
        }

        void operator () (IJournalViewModel::quest_id id, IJournalViewModel::utf8_span name)
        {
            BookTypesetter::Style* style = typesetter->createHotStyle (body_style, MyGUI::Colour::Black, linkHot, linkActive, id);

            typesetter->write (style, name);
            typesetter->lineBreak ();
        }
    };
}

typedef TypesetBook::ptr book;

JournalBooks::JournalBooks (IJournalViewModel::ptr Model) :
    Model (Model)
{
}

book JournalBooks::createEmptyJournalBook ()
{
    BookTypesetter::ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("EB Garamond", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    typesetter->write (header, to_utf8_span ("You have no journal entries!"));
    typesetter->lineBreak ();
    typesetter->write (body, to_utf8_span ("You should have gone though the starting quest and got an initial quest."));

    BookTypesetter::Style* big    = typesetter->createStyle ("EB Garamond 24", MyGUI::Colour::Black);
    BookTypesetter::Style* test   = typesetter->createStyle ("MonoFont", MyGUI::Colour::Blue);

    typesetter->sectionBreak (20);
    typesetter->write (body, to_utf8_span (
        "The layout engine doesn't currently support aligning fonts to "
        "their baseline within a single line so the following text looks "
        "funny. In order to properly implement it, a stupidly simple "
        "change is needed in MyGUI to report the where the baseline is for "
        "a particular font"
    ));

    typesetter->sectionBreak (20);
    typesetter->write (big, to_utf8_span ("big text g"));
    typesetter->write (body, to_utf8_span (" проверяем только в дебаге"));
    typesetter->write (body, to_utf8_span (" normal g"));
    typesetter->write (big, to_utf8_span (" done g"));

    typesetter->sectionBreak (20);
    typesetter->write (test, to_utf8_span (
        "int main (int argc,\n"
        "          char ** argv)\n"
        "{\n"
        "    print (\"hello world!\\n\");\n"
        "    return 0;\n"
        "}\n"
    ));

    return typesetter->complete ();
}

book JournalBooks::createJournalBook ()
{
    BookTypesetter::ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("EB Garamond", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    Model->visitJournalEntries (0, addJournalEntry (typesetter, body, header, true));

    return typesetter->complete ();
}

book JournalBooks::createTopicBook (uintptr_t topicId)
{
    BookTypesetter::ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("EB Garamond", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    Model->visitTopicName (topicId, addTopicName (typesetter, header));

    intptr_t contentId = typesetter->add_content (to_utf8_span (", \""));

    Model->visitTopicEntries (topicId, addTopicEntry (typesetter, body, header, contentId));

    return typesetter->complete ();
}

book JournalBooks::createQuestBook (uintptr_t questId)
{
    BookTypesetter::ptr typesetter = createTypesetter ();

    BookTypesetter::Style* header = typesetter->createStyle ("EB Garamond", MyGUI::Colour (0.60f, 0.00f, 0.00f));
    BookTypesetter::Style* body   = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    Model->visitQuestName (questId, addQuestName (typesetter, header));

    Model->visitJournalEntries (questId, addJournalEntry (typesetter, body, header, false));

    return typesetter->complete ();
}

book JournalBooks::createTopicIndexBook ()
{
    BookTypesetter::ptr typesetter = BookTypesetter::create (92, 250);

    typesetter->setSectionAlignment (BookTypesetter::alignCenter);

    BookTypesetter::Style* body   = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

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
    BookTypesetter::ptr typesetter = BookTypesetter::create (0x7FFFFFFF, 0x7FFFFFFF);
    BookTypesetter::Style* style = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    Model->visitTopicNamesStartingWith (character, addTopicLink (typesetter, style));

    return typesetter->complete ();
}

book JournalBooks::createQuestIndexBook (bool activeOnly)
{
    BookTypesetter::ptr typesetter = BookTypesetter::create (0x7FFFFFFF, 0x7FFFFFFF);
    BookTypesetter::Style* base = typesetter->createStyle ("EB Garamond", MyGUI::Colour::Black);

    Model->visitQuestNames (activeOnly, addQuestLink (typesetter, base));

    return typesetter->complete ();
}

BookTypesetter::ptr JournalBooks::createTypesetter ()
{
    //TODO: determine page size from layout...
    return BookTypesetter::create (240, 300);
}
