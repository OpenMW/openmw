#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include "itemmodel.hpp"

#include "../mwworld/containerstore.hpp"

namespace MyGUI
{
    class Gui;
    class Widget;
}

namespace MWGui
{
    class ContainerWindow;
    class ItemView;
    class SortFilterItemModel;
}

namespace MWGui
{
    class ContainerWindow : public WindowBase, public ReferenceInterface, public MWWorld::ContainerStoreListener
    {
    public:
        ContainerWindow(DragAndDrop* dragAndDrop);

        void setPtr(const MWWorld::Ptr& container) override;
        void onClose() override;
        void clear() override { resetReference(); }

        void onFrame(float dt) override;

        void resetReference() override;

        void onDeleteCustomData(const MWWorld::Ptr& ptr) override;

        void treatNextOpenAsLoot() { mTreatNextOpenAsLoot = true; }

        void itemAdded(const MWWorld::ConstPtr& item, int count) override;
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override;

        std::string_view getWindowIdForLua() const override { return "Container"; }

    private:
        DragAndDrop* mDragAndDrop;

        MWGui::ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        ItemModel* mModel;
        int mSelectedItem;
        bool mUpdateNextFrame;
        bool mTreatNextOpenAsLoot;
        MyGUI::Button* mDisposeCorpseButton;
        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        void onItemSelected(int index);
        void onBackgroundSelected();
        void dragItem(MyGUI::Widget* sender, int count);
        void dropItem();
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeAllButtonClicked(MyGUI::Widget* _sender);
        void onDisposeCorpseButtonClicked(MyGUI::Widget* sender);

        /// @return is taking the item allowed?
        bool onTakeItem(const ItemStack& item, int count);

        void onReferenceUnavailable() override;
    };
}
#endif // CONTAINER_H
