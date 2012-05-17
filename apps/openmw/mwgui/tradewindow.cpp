#include "tradewindow.hpp"

namespace MWGui
{
    TradeWindow::TradeWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_trade_window_layout.xml", parWindowManager),
        ContainerBase(NULL) // no drag&drop
    {
    }
}
