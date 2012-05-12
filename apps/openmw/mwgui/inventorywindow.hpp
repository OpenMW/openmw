#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "container.hpp"
#include "window_pinnable_base.hpp"

namespace MWWorld
{
    class Environment;
}

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{
    class InventoryWindow : public ContainerBase, public WindowPinnableBase
    {
        public:
            InventoryWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

            void openInventory();

        protected:
            MyGUI::Widget* mAvatar;
            MyGUI::TextBox* mArmorRating;
            MyGUI::ProgressBar* mEncumbranceBar;
            MyGUI::TextBox* mEncumbranceText;

            MyGUI::Button* mFilterAll;
            MyGUI::Button* mFilterWeapon;
            MyGUI::Button* mFilterApparel;
            MyGUI::Button* mFilterMagic;
            MyGUI::Button* mFilterMisc;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onPinToggled();
    };
}
#endif // Inventory_H
