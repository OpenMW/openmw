#include "window_base.hpp"
#include "../mwworld/environment.hpp"
#include "window_manager.hpp"

using namespace MWGui;

WindowBase::WindowBase(const std::string& parLayout, MWWorld::Environment& parEnvironment)
  : Layout(parLayout)
  , environment(parEnvironment)
{
}

void WindowBase::open()
{
}

void WindowBase::center()
{
    // Centre dialog
    MyGUI::IntSize gameWindowSize = environment.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}
