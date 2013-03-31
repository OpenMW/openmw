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

        // basic types (inclusive)
        static const int Filter_All = (1<<0);
        static const int Filter_Weapon = (1<<1);
        static const int Filter_Apparel = (1<<2);
        static const int Filter_Ingredients = (1<<3);
        static const int Filter_Misc = (1<<4);

        // special filtering (exclusive)
        static const int Filter_Magic = (1<<5);
        static const int Filter_NoMagic = (1<<6);
        static const int Filter_ChargedSoulstones = (1<<7);

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
        void setFilter(int filter); ///< set category filter
        void drawItems();

        /// fired when an item was moved by drag&drop. \n
        /// if it was removed from this container, count will be negative.
        virtual void notifyItemDragged(MWWorld::Ptr item, int count) {}

    protected:
        bool mDisplayEquippedItems;
        bool mHighlightEquippedItems;

        MyGUI::ScrollView* mItemView;
        MyGUI::Widget* mContainerWidget;

        MyGUI::Widget* mSelectedItem;

        DragAndDrop* mDragAndDrop;

        int mFilter;

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
