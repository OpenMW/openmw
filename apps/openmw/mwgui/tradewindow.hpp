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

        void sellToNpc(
            const MWWorld::Ptr& item, int count, bool boughtItem); ///< only used for adjusting the gold balance
        void buyFromNpc(
            const MWWorld::Ptr& item, int count, bool soldItem); ///< only used for adjusting the gold balance

        void updateOffer();

        void onItemSelected(int index);
        void sellItem(MyGUI::Widget* sender, int count);

        void borrowItem(int index, size_t count);
        void returnItem(int index, size_t count);

        int getMerchantServices();

        void onFilterChanged(MyGUI::Widget* _sender);
        void onNameFilterChanged(MyGUI::EditBox* _sender);
        void onOfferButtonClicked(MyGUI::Widget* _sender);
        void onAccept(MyGUI::EditBox* sender);
        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onMaxSaleButtonClicked(MyGUI::Widget* _sender);
        void onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onBalanceButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onBalanceValueChanged(int value);
        void onRepeatClick(MyGUI::Widget* widget, MyGUI::ControllerItem* controller);
        void onOfferSubmitted(MyGUI::Widget* _sender, int offerAmount);

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
