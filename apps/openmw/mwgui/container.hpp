#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include "../mwworld/esmstore.hpp"

#include "window_base.hpp"
#include "referenceinterface.hpp"

#include "../mwclass/container.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"


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
    class ContainerWindow;
    class ContainerBase;
}


namespace MWGui
{
    class DragAndDrop
    {
    public:
        bool mIsOnDragAndDrop;
        MyGUI::Widget* mDraggedWidget;
        MyGUI::Widget* mDragAndDropWidget;
        ContainerBase* mDraggedFrom;
        int mDraggedCount;
    };

    class ContainerBase : public ReferenceInterface
    {
    public:
        ContainerBase(DragAndDrop* dragAndDrop);
        virtual ~ContainerBase();

        enum Filter
        {
            Filter_All = 0x01,
            Filter_Weapon = 0x02,
            Filter_Apparel = 0x03,
            Filter_Magic = 0x04,
            Filter_Misc = 0x05,

            Filter_Ingredients = 0x06
        };

        enum ItemState
        {
            ItemState_Normal = 0x01,
            ItemState_Equipped = 0x02,
            ItemState_Barter = 0x03
        };

        void setWidgets(MyGUI::Widget* containerWidget, MyGUI::ScrollView* itemView); ///< only call once

        void addBarteredItem(MWWorld::Ptr item, int count);
        void addItem(MWWorld::Ptr item, int count);

        void transferBoughtItems(); ///< transfer bought items into the inventory
        void returnBoughtItems(MWWorld::ContainerStore& store); ///< return bought items into the specified ContainerStore

        MWWorld::ContainerStore& getContainerStore();
        MWWorld::ContainerStore& getBoughtItems() { return mBoughtItems; }

        void openContainer(MWWorld::Ptr container);
        void setFilter(Filter filter); ///< set category filter
        void drawItems();

    protected:
        bool mDisplayEquippedItems;
        bool mHighlightEquippedItems;

        MyGUI::ScrollView* mItemView;
        MyGUI::Widget* mContainerWidget;

        MyGUI::Widget* mSelectedItem;

        DragAndDrop* mDragAndDrop;

        Filter mFilter;

        // bought items are put in a separate ContainerStore so that they don't stack with other (not bought) items.
        MWWorld::ContainerStore mBoughtItems;

        void onSelectedItem(MyGUI::Widget* _sender);
        void onContainerClicked(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);

        /// start dragging an item (drag & drop)
        void startDragItem(MyGUI::Widget* _sender, int count);

        /// sell an item from this container
        void sellItem(MyGUI::Widget* _sender, int count);

        /// sell an item from this container, that was previously just bought
        void sellAlreadyBoughtItem(MyGUI::Widget* _sender, int count);

        std::string getCountString(const int count);

        virtual bool isTradeWindow() { return false; }
        virtual bool isInventory() { return false; }
        virtual std::vector<MWWorld::Ptr> getEquippedItems();
        virtual void _unequipItem(MWWorld::Ptr item) { ; }

        virtual bool isTrading() { return false; }

        virtual void onSelectedItemImpl(MWWorld::Ptr item) { ; }

        virtual std::vector<MWWorld::Ptr> itemsToIgnore() { return std::vector<MWWorld::Ptr>(); }

        virtual void notifyContentChanged() { ; }
    };

    class ContainerWindow : public ContainerBase, public WindowBase
    {
    public:
        ContainerWindow(MWBase::WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

        virtual ~ContainerWindow();

        void open(MWWorld::Ptr container, bool loot=false);

    protected:
        MyGUI::Button* mDisposeCorpseButton;
        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        void onWindowResize(MyGUI::Window* window);
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeAllButtonClicked(MyGUI::Widget* _sender);
        void onDisposeCorpseButtonClicked(MyGUI::Widget* sender);

        virtual void onReferenceUnavailable();
    };
}
#endif // CONTAINER_H
