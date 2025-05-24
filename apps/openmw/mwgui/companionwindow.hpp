#ifndef OPENMW_MWGUI_COMPANIONWINDOW_H
#define OPENMW_MWGUI_COMPANIONWINDOW_H

#include "companionitemmodel.hpp"
#include "itemmodel.hpp"
#include "referenceinterface.hpp"
#include "windowbase.hpp"

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

        bool exit() override;

        void resetReference() override;

        void setPtr(const MWWorld::Ptr& actor) override;
        void onFrame(float dt) override;
        void clear() override { resetReference(); }

        std::string_view getWindowIdForLua() const override { return "Companion"; }

        MWGui::ItemView* getItemView() { return mItemView; }
        ItemModel* getModel() { return mModel; }

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

        void onReferenceUnavailable() override;
    };

}

#endif
