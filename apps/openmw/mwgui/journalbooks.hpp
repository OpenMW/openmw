#ifndef MWGUI_JOURNALBOOKS_HPP
#define MWGUI_JOURNALBOOKS_HPP

#include "bookpage.hpp"
#include "journalviewmodel.hpp"

namespace MWGui
{
    struct JournalBooks
    {
        typedef ITypesetBook::ptr book;
        IJournalViewModel::ptr Model;

        JournalBooks (IJournalViewModel::ptr Model);

        book createEmptyJournalBook ();
        book createJournalBook ();
        book createTopicBook (uintptr_t topicId);
        book createQuestBook (uintptr_t questId);
        book createTopicIndexBook ();
        book createTopicIndexBook (char character);
        book createQuestIndexBook (bool showAll);

    private:
        IBookTypesetter::ptr createTypesetter ();
    };
}

#endif // MWGUI_JOURNALBOOKS_HPP
