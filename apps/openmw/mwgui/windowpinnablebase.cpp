#include "windowpinnablebase.hpp"

#include "exposedwindow.hpp"

namespace MWGui
{
    WindowPinnableBase::WindowPinnableBase(const std::string& parLayout)
      : WindowBase(parLayout), mPinned(false)
    {
        Window* window = mMainWidget->castType<Window>();
        mPinButton = window->getSkinWidget ("Button");

        mPinButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &WindowPinnableBase::onPinButtonPressed);
    }

    void WindowPinnableBase::onPinButtonPressed(MyGUI::Widget* _sender, int left, int top, MyGUI::MouseButton id)
    {
        if (id != MyGUI::MouseButton::Left)
            return;

        mPinned = !mPinned;

        if (mPinned)
            mPinButton->changeWidgetSkin ("PinDown");
        else
            mPinButton->changeWidgetSkin ("PinUp");

        onPinToggled();
    }

    void WindowPinnableBase::setPinned(bool pinned)
    {
        if (pinned != mPinned)
            onPinButtonPressed(mPinButton, 0, 0, MyGUI::MouseButton::Left);
    }

    void WindowPinnableBase::setPinButtonVisible(bool visible)
    {
        mPinButton->setVisible(visible);
    }
}
