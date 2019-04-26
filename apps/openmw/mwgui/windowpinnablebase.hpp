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
        void onPinButtonPressed(MyGUI::Widget* _sender, int left, int top, MyGUI::MouseButton id);

    protected:
        virtual void onPinToggled() = 0;

        MyGUI::Widget* mPinButton;
        bool mPinned;
    };
}

#endif
