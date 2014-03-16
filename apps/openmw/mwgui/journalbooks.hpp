#ifndef MWGUI_JOURNALBOOKS_HPP
#define MWGUI_JOURNALBOOKS_HPP

#include "bookpage.hpp"
#include "journalviewmodel.hpp"

namespace MWGui
{
    MWGui::BookTypesetter::Utf8Span to_utf8_span (char const * text)
    {
        typedef MWGui::BookTypesetter::Utf8Point point;

        point begin = reinterpret_cast <point> (text);

        return MWGui::BookTypesetter::Utf8Span (begin, begin + strlen (text));
    }

    struct JournalBooks
    {
        typedef TypesetBook::Ptr Book;
        JournalViewModel::Ptr mModel;

        JournalBooks (JournalViewModel::Ptr model);

        Book createEmptyJournalBook ();
        Book createJournalBook ();
        Book createTopicBook (uintptr_t topicId);
        Book createQuestBook (uintptr_t questId);
        Book createTopicIndexBook ();
        Book createTopicIndexBook (char character);
        Book createQuestIndexBook (bool showAll);

    private:
        BookTypesetter::Ptr createTypesetter ();
    };
}

#endif // MWGUI_JOURNALBOOKS_HPP
