#ifndef MWGUI_JOURNAL_H
#define MWGUI_JOURNAL_H

#include "windowbase.hpp"

#include <components/to_utf8/to_utf8.hpp>

#include <memory>

namespace MWBase
{
    class WindowManager;
}

namespace MWGui
{
    struct JournalViewModel;

    struct JournalWindow : public BookWindowBase
    {
        JournalWindow();

        /// construct a new instance of the one JournalWindow implementation
        static std::unique_ptr<JournalWindow> create(
            std::shared_ptr<JournalViewModel> model, bool questList, ToUTF8::FromType encoding);

        /// destroy this instance of the JournalWindow implementation
        virtual ~JournalWindow() {}

        /// show/hide the journal window
        void setVisible(bool newValue) override = 0;

        std::string_view getWindowIdForLua() const override { return "Journal"; }

        std::vector<MyGUI::Button*> mButtons;
        int mSelectedQuest = 0;
        int mSelectedIndex = 0;
        void setIndexControllerFocus(int index, bool focused);
        void setControllerFocusedQuest(int index);
    };
}

#endif
