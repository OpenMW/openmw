#include "alchemywindow.hpp"

namespace MWGui
{
    AlchemyWindow::AlchemyWindow(WindowManager& parWindowManager)
        : WindowBase("openmw_alchemy_window_layout.xml", parWindowManager)
    {
        center();
    }
}
