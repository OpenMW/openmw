#include "window_base.hpp"
#include "window_manager.hpp"

#include <components/settings/settings.hpp>

using namespace MWGui;

WindowBase::WindowBase(const std::string& parLayout, WindowManager& parWindowManager)
  : Layout(parLayout)
  , mWindowManager(parWindowManager)
{
}

void WindowBase::open()
{
}

void WindowBase::setVisible(bool visible)
{
    bool wasVisible = mMainWidget->getVisible();
    mMainWidget->setVisible(visible);

    if (!wasVisible && visible)
        open();
}

void WindowBase::center()
{
    // Centre dialog

    // MyGUI::IntSize gameWindowSize = MyGUI::RenderManager::getInstance().getViewSize();
    // Note by scrawl: The following works more reliably in the case when the window was _just_
    // resized and MyGUI RenderManager doesn't know about the new size yet
    MyGUI::IntSize gameWindowSize = MyGUI::IntSize(Settings::Manager::getInt("resolution x", "Video"),
            Settings::Manager::getInt("resolution y", "Video"));

    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}
