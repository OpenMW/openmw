#include "inventorywindow.hpp"
#include <iterator>
#include <algorithm>
#include "window_manager.hpp"
#include "widgets.hpp"

#include "../mwbase/environment.hpp"
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

    InventoryWindow::InventoryWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
        : ContainerBase(parWindowManager,dragAndDrop,"openmw_inventory_window_layout.xml")
    {
        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &InventoryWindow::onWindowResize);

        getWidget(mAvatar, "Avatar");
        getWidget(mEncumbranceBar, "EncumbranceBar");
        getWidget(mEncumbranceText, "EncumbranceBarT");
        getWidget(mFilterAll, "AllButton");
        getWidget(mFilterWeapon, "WeaponButton");
        getWidget(mFilterApparel, "ApparelButton");
        getWidget(mFilterMagic, "MagicButton");
        getWidget(mFilterMisc, "MiscButton");
    }

    void InventoryWindow::openInventory()
    {
        open(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

        onWindowResize(static_cast<MyGUI::Window*>(mMainWidget));
    }

    void InventoryWindow::onWindowResize(MyGUI::Window* _sender)
    {
        drawItems();
    }

}
