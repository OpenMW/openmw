#ifndef OPENMW_MWGUI_COMPANIONWINDOW_H
#define OPENMW_MWGUI_COMPANIONWINDOW_H

#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace MWGui
{
    namespace Widgets
    {
        class MWDynamicStat;
    }

    class MessageBoxManager;
    class ItemView;
    class DragAndDrop;
    class SortFilterItemModel;
    class CompanionItemModel;

    class CompanionWindow : public WindowBase, public ReferenceInterface
    {
    public:
        CompanionWindow(DragAndDrop* dragAndDrop, MessageBoxManager* manager);

        virtual bool exit();

        virtual void resetReference();

        void setPtr(const MWWorld::Ptr& npc);
        void onFrame (float dt);
        void clear() { resetReference(); }

    private:
        ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        CompanionItemModel* mModel;
        int mSelectedItem;

        DragAndDrop* mDragAndDrop;

        MyGUI::Button* mCloseButton;
        MyGUI::EditBox* mFilterEdit;
        MyGUI::TextBox* mProfitLabel;
        Widgets::MWDynamicStat* mEncumbranceBar;
        MessageBoxManager* mMessageBoxManager;

        void onItemSelected(int index);
        void onNameFilterChanged(MyGUI::EditBox* _sender);
        void onBackgroundSelected();
        void dragItem(MyGUI::Widget* sender, int count);

        void onMessageBoxButtonClicked(int button);

        void updateEncumbranceBar();

        void onCloseButtonClicked(MyGUI::Widget* _sender);

        virtual void onReferenceUnavailable();
    };

}

#endif
