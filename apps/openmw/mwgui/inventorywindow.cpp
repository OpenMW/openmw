#include "inventorywindow.hpp"
#include <iterator>
#include <algorithm>
#include "window_manager.hpp"
#include "widgets.hpp"

#include "../mwworld/environment.hpp"
#include "../mwworld/manualref.hpp"
#include <cmath>
#include <algorithm>
#include <iterator>

#include <assert.h>
#include <iostream>
#include "../mwclass/container.hpp"
#include "../mwworld/containerstore.hpp"
#include <boost/lexical_cast.hpp>
#include "../mwworld/class.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"
namespace MWGui
{

    InventoryWindow::InventoryWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,DragAndDrop* dragAndDrop)
        :ContainerWindow(parWindowManager,environment,dragAndDrop,"openmw_inventory_window_layout.xml")
    {
    }

    void InventoryWindow::openInventory()
    {
        open(mEnvironment.mWorld->getPlayer().getPlayer());
    }

}