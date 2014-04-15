#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "../mwrender/characterpreview.hpp"

#include "windowpinnablebase.hpp"
#include "widgets.hpp"
#include "mode.hpp"

namespace MWGui
{
    class ItemView;
    class SortFilterItemModel;
    class TradeItemModel;
    class DragAndDrop;
    class ItemModel;

    class InventoryWindow : public WindowPinnableBase
    {
        public:
            InventoryWindow(DragAndDrop* dragAndDrop);

            virtual void open();

            void doRenderUpdate();

            /// start trading, disables item drag&drop
            void setTrading(bool trading);

            void onFrame();

            void pickUpObject (MWWorld::Ptr object);

            MWWorld::Ptr getAvatarSelectedItem(int x, int y);

            void rebuildAvatar() {
                mPreview.rebuild();
            }

            TradeItemModel* getTradeModel();
            ItemModel* getModel();

            void updateItemView();

            void updatePlayer();

            void useItem(const MWWorld::Ptr& ptr);

            void setGuiMode(GuiMode mode);

        private:
            DragAndDrop* mDragAndDrop;

            bool mPreviewDirty;
            size_t mSelectedItem;

            MWWorld::Ptr mPtr;

            MWGui::ItemView* mItemView;
            SortFilterItemModel* mSortModel;
            TradeItemModel* mTradeModel;

            MyGUI::Widget* mAvatar;
            MyGUI::ImageBox* mAvatarImage;
            MyGUI::TextBox* mArmorRating;
            Widgets::MWDynamicStat* mEncumbranceBar;

            MyGUI::Widget* mLeftPane;
            MyGUI::Widget* mRightPane;

            MyGUI::Button* mFilterAll;
            MyGUI::Button* mFilterWeapon;
            MyGUI::Button* mFilterApparel;
            MyGUI::Button* mFilterMagic;
            MyGUI::Button* mFilterMisc;

            MWWorld::Ptr mSkippedToEquip;

            GuiMode mGuiMode;

            int mLastXSize;
            int mLastYSize;

            MWRender::InventoryPreview mPreview;

            bool mTrading;

            void onItemSelected(int index);
            void onItemSelectedFromSourceModel(int index);

            void onBackgroundSelected();

            void sellItem(MyGUI::Widget* sender, int count);
            void dragItem(MyGUI::Widget* sender, int count);

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onAvatarClicked(MyGUI::Widget* _sender);
            void onPinToggled();

            void updateEncumbranceBar();
            void notifyContentChanged();

            void adjustPanes();
    };
}

#endif // Inventory_H
