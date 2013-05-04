#ifndef MWGUI_DIALOGE_HISTORY_H
#define MWGUI_DIALOGE_HISTORY_H

#include "keywordsearch.hpp"

#include <platform/stdint.h>

namespace MWGui
{
    class DialogueHistoryViewModel
    {
    public:
        DialogueHistoryViewModel();

    private:
        typedef KeywordSearch <std::string, intptr_t> KeywordSearchT;

        mutable bool             mKeywordSearchLoaded;
        mutable KeywordSearchT mKeywordSearch;

    };
}
#endif

