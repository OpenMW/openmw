#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "../mwrender/characterpreview.hpp"

#include "container.hpp"
#include "window_pinnable_base.hpp"

namespace MWGui
{
    class InventoryWindow : public ContainerBase, public WindowPinnableBase
    {
        public:
            InventoryWindow(MWBase::WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

            virtual void open();

            /// start trading, disables item drag&drop
            void startTrade();

            void onFrame();

            void pickUpObject (MWWorld::Ptr object);

            int getPlayerGold();

            MyGUI::IntCoord getAvatarScreenCoord();

            MWWorld::Ptr getAvatarSelectedItem(int x, int y);

            void rebuildAvatar() {
                mPreview.rebuild();
            }

        protected:
            MyGUI::Widget* mAvatar;
            MyGUI::ImageBox* mAvatarImage;
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

            int mLastXSize;
            int mLastYSize;

            MWRender::InventoryPreview mPreview;

            bool mTrading;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onAvatarClicked(MyGUI::Widget* _sender);
            void onPinToggled();

            void updateEncumbranceBar();

            virtual bool isTrading() { return mTrading; }
            virtual bool isInventory() { return true; }
            virtual void _unequipItem(MWWorld::Ptr item);

            virtual void onReferenceUnavailable() { ; }

            virtual void notifyContentChanged();
    };
}

#endif // Inventory_H
