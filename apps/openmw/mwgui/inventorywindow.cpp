#include "inventorywindow.hpp"

namespace MWGui
{

    InventoryWindow::InventoryWindow(WindowManager& parWindowManager,MWWorld::Environment& environment)
        :ContainerWindow(parWindowManager,environment,"openmw_inventory_window_layout.xml")
    {
    }

}