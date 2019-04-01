#ifndef MWGUI_JOURNAL_H
#define MWGUI_JOURNAL_H

#include "windowbase.hpp"

#include <components/to_utf8/to_utf8.hpp>

#include <memory>

namespace MWBase { class WindowManager; }

namespace MWGui
{
    struct JournalViewModel;

    struct JournalWindow : public BookWindowBase
    {
        JournalWindow();

        /// construct a new instance of the one JournalWindow implementation
        static JournalWindow * create (std::shared_ptr <JournalViewModel> Model, bool questList, ToUTF8::FromType encoding);

        /// destroy this instance of the JournalWindow implementation
        virtual ~JournalWindow () {};

        /// show/hide the journal window
        virtual void setVisible (bool newValue) = 0;
    };
}

#endif
