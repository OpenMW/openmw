#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "container.hpp"
#include "window_pinnable_base.hpp"

namespace MWGui
{
    class InventoryWindow : public ContainerBase, public WindowPinnableBase
    {
        public:
            InventoryWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

            virtual void open();

            /// start trading, disables item drag&drop
            void startTrade();

            void onFrame();

            int getPlayerGold();

        protected:
            MyGUI::Widget* mAvatar;
            MyGUI::TextBox* mArmorRating;
            MyGUI::ProgressBar* mEncumbranceBar;
            MyGUI::TextBox* mEncumbranceText;

            MyGUI::Widget* mLeftPane;
            MyGUI::Widget* mRightPane;

            MyGUI::Button* mFilterAll;
            MyGUI::Button* mFilterWeapon;
            MyGUI::Button* mFilterApparel;
            MyGUI::Button* mFilterMagic;
            MyGUI::Button* mFilterMisc;

            bool mTrading;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onAvatarClicked(MyGUI::Widget* _sender);
            void onPinToggled();

            void updateEncumbranceBar();

            virtual bool isTrading() { return mTrading; }
            virtual bool isInventory() { return true; }
            virtual std::vector<MWWorld::Ptr> getEquippedItems();
            virtual void _unequipItem(MWWorld::Ptr item);
    };
}

#endif // Inventory_H
