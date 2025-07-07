#ifndef MGUI_Inventory_H
#define MGUI_Inventory_H

#include "mode.hpp"
#include "windowpinnablebase.hpp"

#include "../mwrender/characterpreview.hpp"
#include "../mwworld/ptr.hpp"

namespace osg
{
    class Group;
}

namespace Resource
{
    class ResourceSystem;
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
        InventoryWindow(DragAndDrop* dragAndDrop, osg::Group* parent, Resource::ResourceSystem* resourceSystem);

        void onOpen() override;

        /// start trading, disables item drag&drop
        void setTrading(bool trading);

        void onFrame(float dt) override;

        void pickUpObject(MWWorld::Ptr object);

        MWWorld::Ptr getAvatarSelectedItem(int x, int y);

        void rebuildAvatar();

        SortFilterItemModel* getSortFilterModel();
        TradeItemModel* getTradeModel();
        ItemModel* getModel();

        void updateItemView();

        void updatePlayer();

        void clear() override;

        void useItem(const MWWorld::Ptr& ptr, bool force = false);

        void setGuiMode(GuiMode mode);

        /// Cycle to previous/next weapon
        void cycle(bool next);

        std::string_view getWindowIdForLua() const override { return "Inventory"; }

        ControllerButtonStr* getControllerButtons() override;

    protected:
        void onTitleDoubleClicked() override;
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void setActiveControllerWindow(bool active) override;

    private:
        DragAndDrop* mDragAndDrop;

        int mSelectedItem;
        std::optional<int> mEquippedStackableCount;

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

        MyGUI::EditBox* mFilterEdit;

        GuiMode mGuiMode;

        int mLastXSize;
        int mLastYSize;

        std::unique_ptr<MyGUI::ITexture> mPreviewTexture;
        std::unique_ptr<MWRender::InventoryPreview> mPreview;

        bool mTrading;
        float mUpdateTimer;

        void toggleMaximized();

        void onItemSelected(int index);
        void onItemSelectedFromSourceModel(int index);

        void onBackgroundSelected();

        enum ControllerAction
        {
            None = 0,
            Use,
            Give,
            Sell,
            Drop,
        };
        ControllerAction mPendingControllerAction;

        void sellItem(MyGUI::Widget* sender, int count);
        void dragItem(MyGUI::Widget* sender, int count);
        void giveItem(MyGUI::Widget* sender, int count);
        void dropItem(MyGUI::Widget* sender, int count);

        void onWindowResize(MyGUI::Window* _sender);
        void onFilterChanged(MyGUI::Widget* _sender);
        void onNameFilterChanged(MyGUI::EditBox* _sender);
        void onAvatarClicked(MyGUI::Widget* _sender);
        void onPinToggled() override;

        void updateEncumbranceBar();
        void notifyContentChanged();
        void dirtyPreview();
        void updatePreviewSize();
        void updateArmorRating();

        MyGUI::IntSize getPreviewViewportSize() const;
        osg::Vec2f mapPreviewWindowToViewport(int x, int y) const;

        void adjustPanes();

        /// Unequips count items from mSelectedItem, if it is equipped, and then updates mSelectedItem in case the items
        /// were re-stacked
        void ensureSelectedItemUnequipped(int count);
    };
}

#endif // Inventory_H
