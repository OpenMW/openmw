#ifndef MWGUI_WINDOW_PINNABLE_BASE_H
#define MWGUI_WINDOW_PINNABLE_BASE_H

#include "windowbase.hpp"

namespace MWGui
{
    class WindowManager;

    class WindowPinnableBase: public WindowBase
    {
    public:
        WindowPinnableBase(const std::string& parLayout);
        bool pinned() { return mPinned; }
        void setPinned (bool pinned);
        void setPinButtonVisible(bool visible);

    private:
        void onPinButtonClicked(MyGUI::Widget* _sender);
        void onDoubleClick(MyGUI::Widget* _sender);

    protected:
        virtual void onPinToggled() = 0;
        virtual void onTitleDoubleClicked() = 0;

        MyGUI::Widget* mPinButton;
        bool mPinned;
    };
}

#endif
