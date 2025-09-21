#ifndef MWGUI_TRADEWINDOW_H
#define MWGUI_TRADEWINDOW_H

#include "referenceinterface.hpp"
#include "windowbase.hpp"

#include "../mwworld/containerstore.hpp"

namespace Gui
{
    class NumericEditBox;
}

namespace MyGUI
{
    class ControllerItem;
}

namespace MWGui
{
    class ItemView;
    class SortFilterItemModel;
    class TradeItemModel;

    class TradeWindow : public WindowBase, public ReferenceInterface, public MWWorld::ContainerStoreListener
    {
    public:
        TradeWindow();

        void setPtr(const MWWorld::Ptr& actor) override;

        void onClose() override;
        void onFrame(float dt) override;
        void clear() override { resetReference(); }

        bool exit() override;

        void resetReference() override;

        void onDeleteCustomData(const MWWorld::Ptr& ptr) override;

        void updateItemView();

        void itemAdded(const MWWorld::ConstPtr& item, int count) override;
        void itemRemoved(const MWWorld::ConstPtr& item, int count) override;

        typedef MyGUI::delegates::MultiDelegate<> EventHandle_TradeDone;
        EventHandle_TradeDone eventTradeDone;

        std::string_view getWindowIdForLua() const override { return "Trade"; }

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void setActiveControllerWindow(bool active) override;

    private:
        friend class InventoryWindow;

        ItemView* mItemView;
        SortFilterItemModel* mSortModel;
        TradeItemModel* mTradeModel;

        static const float sBalanceChangeInitialPause; // in seconds
        static const float sBalanceChangeInterval; // in seconds

        MyGUI::Button* mFilterAll;
        MyGUI::Button* mFilterWeapon;
        MyGUI::Button* mFilterApparel;
        MyGUI::Button* mFilterMagic;
        MyGUI::Button* mFilterMisc;

        MyGUI::EditBox* mFilterEdit;

        MyGUI::Button* mIncreaseButton;
        MyGUI::Button* mDecreaseButton;
        MyGUI::TextBox* mTotalBalanceLabel;
        Gui::NumericEditBox* mTotalBalance;

        MyGUI::Widget* mBottomPane;

        MyGUI::Button* mMaxSaleButton;
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mOfferButton;
        MyGUI::TextBox* mPlayerGold;
        MyGUI::TextBox* mMerchantGold;

        int mItemToSell;

        int mCurrentBalance;
        int mCurrentMerchantOffer;

        bool mUpdateNextFrame;

        void updateOffer();

        void onItemSelected(int index);
        void sellItem(MyGUI::Widget* sender, std::size_t count);

        void borrowItem(int index, size_t count);
        void returnItem(int index, size_t count);

        int getMerchantServices();

        void onFilterChanged(MyGUI::Widget* sender);
        void onNameFilterChanged(MyGUI::EditBox* sender);
        void onOfferButtonClicked(MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox* sender);
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onMaxSaleButtonClicked(MyGUI::Widget* sender);
        void onIncreaseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onDecreaseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onBalanceButtonReleased(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
        void onBalanceValueChanged(int value);
        void onRepeatClick(MyGUI::Widget* widget, MyGUI::ControllerItem* controller);
        void onOfferSubmitted(MyGUI::Widget* sender, size_t offerAmount);

        void addRepeatController(MyGUI::Widget* widget);

        void onIncreaseButtonTriggered();
        void onDecreaseButtonTriggered();

        void addOrRemoveGold(int gold, const MWWorld::Ptr& actor);

        void updateLabels();

        void onReferenceUnavailable() override;

        int getMerchantGold();
    };
}

#endif
