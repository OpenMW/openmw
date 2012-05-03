#include "scrollwindow.hpp"

using namespace MWGui;

ScrollWindow::ScrollWindow(WindowManager& parWindowManager) :
    WindowBase("openmw_scroll_layout.xml", parWindowManager)
{
    setVisible(false);
    center();
}

void ScrollWindow::open(MWWorld::Ptr scroll)
{
}
