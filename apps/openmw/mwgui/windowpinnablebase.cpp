#include "windowpinnablebase.hpp"

#include "exposedwindow.hpp"

namespace MWGui
{
    WindowPinnableBase::WindowPinnableBase(const std::string& parLayout)
      : WindowBase(parLayout), mPinned(false)
    {
        ExposedWindow* window = static_cast<ExposedWindow*>(mMainWidget);
        mPinButton = window->getSkinWidget ("Button");

        mPinButton->eventMouseButtonClick += MyGUI::newDelegate(this, &WindowPinnableBase::onPinButtonClicked);
    }

    void WindowPinnableBase::onPinButtonClicked(MyGUI::Widget* _sender)
    {
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
            onPinButtonClicked(mPinButton);
    }

    void WindowPinnableBase::setPinButtonVisible(bool visible)
    {
        mPinButton->setVisible(visible);
    }
}
