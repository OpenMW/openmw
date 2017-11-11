#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

#include "itemmodel.hpp"

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
    class ItemView;
    class SortFilterItemModel;
}


namespace MWGui
{
    class ContainerWindow : public WindowBase, public ReferenceInterface
    {
    public:
        ContainerWindow(DragAndDrop* dragAndDrop);

        void setPtr(const MWWorld::Ptr& container);
        virtual void onClose();
        void clear() { resetReference(); }

        void onFrame(float dt) { checkReferenceAvailable(); }

        virtual void resetReference();

    private:
        DragAndDrop* mDragAndDrop;

        MWGui::ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        ItemModel* mModel;
        int mSelectedItem;

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

        virtual void onReferenceUnavailable();
    };
}
#endif // CONTAINER_H
