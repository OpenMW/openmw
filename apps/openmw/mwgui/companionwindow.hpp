#ifndef OPENMW_MWGUI_COMPANIONWINDOW_H
#define OPENMW_MWGUI_COMPANIONWINDOW_H

#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include "../mwworld/containerstore.hpp"

#include <components/misc/notnullptr.hpp>

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
    class ItemTransfer;

    class CompanionWindow : public WindowBase, public ReferenceInterface, public MWWorld::ContainerStoreListener
    {
    public:
        explicit CompanionWindow(DragAndDrop& dragAndDrop, ItemTransfer& itemTransfer, MessageBoxManager* manager);

        bool exit() override;

        void resetReference() override;

        void setPtr(const MWWorld::Ptr& actor) override;
        void onFrame(float dt) override;
        void clear() override { resetReference(); }

        void itemAdded(const MWWorld::ConstPtr& item, int count) override;
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override;

        std::string_view getWindowIdForLua() const override { return "Companion"; }

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void setActiveControllerWindow(bool active) override;

        MWGui::ItemView* getItemView() { return mItemView; }
        CompanionItemModel* getModel() { return mModel; }

    private:
        ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        CompanionItemModel* mModel;
        int mSelectedItem;
        bool mUpdateNextFrame;

        Misc::NotNullPtr<DragAndDrop> mDragAndDrop;
        Misc::NotNullPtr<ItemTransfer> mItemTransfer;

        MyGUI::Button* mCloseButton;
        MyGUI::EditBox* mFilterEdit;
        MyGUI::TextBox* mProfitLabel;
        Widgets::MWDynamicStat* mEncumbranceBar;
        MessageBoxManager* mMessageBoxManager;

        void onItemSelected(int index);
        void onNameFilterChanged(MyGUI::EditBox* sender);
        void onBackgroundSelected();
        void dragItem(MyGUI::Widget* sender, std::size_t count);
        void transferItem(MyGUI::Widget* sender, std::size_t count);

        void onMessageBoxButtonClicked(int button);

        void updateEncumbranceBar();

        void onCloseButtonClicked(MyGUI::Widget* sender);

        void onReferenceUnavailable() override;

        void onOpen() override;

        void onClose() override;
    };

}

#endif
