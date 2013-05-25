#ifndef MWGUI_JOURNALBOOKS_HPP
#define MWGUI_JOURNALBOOKS_HPP

#include "bookpage.hpp"
#include "journalviewmodel.hpp"

namespace MWGui
{
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
