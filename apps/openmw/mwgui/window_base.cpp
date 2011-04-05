#include "window_base.hpp"
#include "window_manager.hpp"

using namespace MWGui;

WindowBase::WindowBase(const std::string& parLayout, WindowManager& parWindowManager)
  : Layout(parLayout)
  , mWindowManager(parWindowManager)
{
}

void WindowBase::open()
{
}

void WindowBase::center()
{
    // Centre dialog
    MyGUI::IntSize gameWindowSize = mWindowManager.getGui()->getViewSize();
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}
