#include "window_pinnable_base.hpp"

#include "../mwbase/windowmanager.hpp"

#include "exposedwindow.hpp"

using namespace MWGui;

WindowPinnableBase::WindowPinnableBase(const std::string& parLayout, MWBase::WindowManager& parWindowManager)
  : WindowBase(parLayout, parWindowManager), mPinned(false), mVisible(false)
{
    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowButtonPressed += MyGUI::newDelegate(this, &WindowPinnableBase::onWindowButtonPressed);

    ExposedWindow* window = static_cast<ExposedWindow*>(mMainWidget);
    mPinButton = window->getSkinWidget ("Button");
}

void WindowPinnableBase::onWindowButtonPressed(MyGUI::Window* sender, const std::string& eventName)
{
    if ("PinToggle" == eventName)
    {
        mPinned = !mPinned;

        if (mPinned)
            mPinButton->changeWidgetSkin ("PinDown");
        else
            mPinButton->changeWidgetSkin ("PinUp");

        onPinToggled();
    }

    eventDone(this);
}
