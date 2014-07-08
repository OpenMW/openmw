#ifndef OPENMW_MWGUI_COMPANIONWINDOW_H
#define OPENMW_MWGUI_COMPANIONWINDOW_H

#include "widgets.hpp"
#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace MWGui
{
    class MessageBoxManager;
    class ItemView;
    class DragAndDrop;
    class SortFilterItemModel;
    class CompanionItemModel;

    class CompanionWindow : public WindowBase, public ReferenceInterface
    {
    public:
        CompanionWindow(DragAndDrop* dragAndDrop, MessageBoxManager* manager);

        virtual void exit();

        virtual void resetReference();

        void open(const MWWorld::Ptr& npc);
        void onFrame ();

    private:
        ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        CompanionItemModel* mModel;
        int mSelectedItem;

        DragAndDrop* mDragAndDrop;

        MyGUI::Button* mCloseButton;
        MyGUI::TextBox* mProfitLabel;
        Widgets::MWDynamicStat* mEncumbranceBar;
        MessageBoxManager* mMessageBoxManager;

        void onItemSelected(int index);
        void onBackgroundSelected();
        void dragItem(MyGUI::Widget* sender, int count);

        void onMessageBoxButtonClicked(int button);

        void updateEncumbranceBar();

        void onCloseButtonClicked(MyGUI::Widget* _sender);

        virtual void onReferenceUnavailable();
    };

}

#endif
