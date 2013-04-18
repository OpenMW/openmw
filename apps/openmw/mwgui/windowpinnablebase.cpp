#include "windowpinnablebase.hpp"

#include "exposedwindow.hpp"

using namespace MWGui;

WindowPinnableBase::WindowPinnableBase(const std::string& parLayout)
  : WindowBase(parLayout), mPinned(false), mVisible(false)
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
