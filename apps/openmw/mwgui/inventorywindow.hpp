#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "windowpinnablebase.hpp"
#include "mode.hpp"

#include "../mwworld/ptr.hpp"

namespace osgViewer
{
    class Viewer;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWRender
{
    class InventoryPreview;
}

namespace MWGui
{
    namespace Widgets
    {
        class MWDynamicStat;
    }

    class ItemView;
    class SortFilterItemModel;
    class TradeItemModel;
    class DragAndDrop;
    class ItemModel;

    class InventoryWindow : public WindowPinnableBase
    {
        public:
            InventoryWindow(DragAndDrop* dragAndDrop, osgViewer::Viewer* viewer, Resource::ResourceSystem* resourceSystem);

            virtual void open();

            /// start trading, disables item drag&drop
            void setTrading(bool trading);

            void onFrame();

            void pickUpObject (MWWorld::Ptr object);

            MWWorld::Ptr getAvatarSelectedItem(int x, int y);

            void rebuildAvatar();

            SortFilterItemModel* getSortFilterModel();
            TradeItemModel* getTradeModel();
            ItemModel* getModel();

            void updateItemView();

            void updatePlayer();

            void useItem(const MWWorld::Ptr& ptr);

            void setGuiMode(GuiMode mode);

            /// Cycle to previous/next weapon
            void cycle(bool next);

        private:
            DragAndDrop* mDragAndDrop;

            int mSelectedItem;

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

            std::auto_ptr<MyGUI::ITexture> mPreviewTexture;
            std::auto_ptr<MWRender::InventoryPreview> mPreview;

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
            void onTitleDoubleClicked();

            void updateEncumbranceBar();
            void notifyContentChanged();
            void dirtyPreview();
            void updatePreviewSize();
            void updateArmorRating();

            void adjustPanes();

            /// Unequips mSelectedItem, if it is equipped, and then updates mSelectedItem in case it was re-stacked
            void ensureSelectedItemUnequipped();
    };
}

#endif // Inventory_H
