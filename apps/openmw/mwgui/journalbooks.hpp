#ifndef MWGUI_JOURNALBOOKS_HPP
#define MWGUI_JOURNALBOOKS_HPP

#include "bookpage.hpp"
#include "journalviewmodel.hpp"

#include <components/to_utf8/to_utf8.hpp>

namespace MWGui
{
    MWGui::BookTypesetter::Utf8Span to_utf8_span(std::string_view text);

    struct JournalBooks
    {
        typedef TypesetBook::Ptr Book;
        JournalViewModel::Ptr mModel;

        JournalBooks(JournalViewModel::Ptr model, ToUTF8::FromType encoding);

        Book createEmptyJournalBook();
        Book createJournalBook();
        Book createTopicBook(uintptr_t topicId);
        Book createQuestBook(std::string_view questName);
        Book createTopicIndexBook();

        ToUTF8::FromType mEncoding;
        int mIndexPagesCount;

    private:
        BookTypesetter::Ptr createTypesetter();
        BookTypesetter::Ptr createLatinJournalIndex();
        BookTypesetter::Ptr createCyrillicJournalIndex();
    };
}

#endif // MWGUI_JOURNALBOOKS_HPP
