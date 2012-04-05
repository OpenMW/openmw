#include "window_pinnable_base.hpp"
#include "window_manager.hpp"

using namespace MWGui;

WindowPinnableBase::WindowPinnableBase(const std::string& parLayout, WindowManager& parWindowManager)
  : WindowBase(parLayout, parWindowManager), mPinned(false), mVisible(false)
{
    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowButtonPressed += MyGUI::newDelegate(this, &WindowPinnableBase::onWindowButtonPressed);
}

void WindowPinnableBase::setVisible(bool b)
{
    // Pinned windows can not be hidden
    if (mPinned && !b)
        return;

    WindowBase::setVisible(b);
    mVisible = b;
}

void WindowPinnableBase::onWindowButtonPressed(MyGUI::Window* sender, const std::string& eventName)
{
    if ("PinToggle" == eventName)
    {
        mPinned = !mPinned;
        onPinToggled();
    }

    eventDone(this);
}

