#include "window_pinnable_base.hpp"
#include "window_manager.hpp"

using namespace MWGui;

WindowPinnableBase::WindowPinnableBase(const std::string& parLayout, WindowManager& parWindowManager)
  : WindowBase(parLayout, parWindowManager), mIsPinned(false)
{
    MyGUI::WindowPtr t = static_cast<MyGUI::WindowPtr>(mMainWidget);
    t->eventWindowButtonPressed += MyGUI::newDelegate(this, &WindowPinnableBase::onWindowButtonPressed);
}

void WindowPinnableBase::setVisible(bool b)
{
    // Pinned windows can not be hidden
    if (mIsPinned && !b)
        return;

    WindowBase::setVisible(b);
}

void WindowPinnableBase::onWindowButtonPressed(MyGUI::Window* sender, const std::string& eventName)
{
    if ("PinToggle" == eventName)
    {
        mIsPinned = !mIsPinned;
    }

    eventDone(this);
}

