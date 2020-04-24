#ifndef MWGUI_TRADEWINDOW_H
#define MWGUI_TRADEWINDOW_H

#include "../mwmechanics/trading.hpp"

#include "referenceinterface.hpp"
#include "windowbase.hpp"

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

    class TradeWindow : public WindowBase, public ReferenceInterface
    {
        public:
            TradeWindow();

            void setPtr(const MWWorld::Ptr& actor);

            void onFrame(float dt);
            void clear() { resetReference(); }

            void borrowItem (int index, size_t count);
            void returnItem (int index, size_t count);

            int getMerchantServices();

            virtual bool exit();

            virtual void resetReference();

            typedef MyGUI::delegates::CMultiDelegate0 EventHandle_TradeDone;
            EventHandle_TradeDone eventTradeDone;

        private:
            ItemView* mItemView;
            SortFilterItemModel* mSortModel;
            TradeItemModel* mTradeModel;
            MWMechanics::Trading mTrading;

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

            void sellToNpc(const MWWorld::Ptr& item, int count, bool boughtItem); ///< only used for adjusting the gold balance
            void buyFromNpc(const MWWorld::Ptr& item, int count, bool soldItem); ///< only used for adjusting the gold balance

            void updateOffer();

            void onItemSelected (int index);
            void sellItem (MyGUI::Widget* sender, int count);

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

            void addRepeatController(MyGUI::Widget* widget);

            void onIncreaseButtonTriggered();
            void onDecreaseButtonTriggered();

            void addOrRemoveGold(int gold, const MWWorld::Ptr& actor);

            void updateLabels();

            virtual void onReferenceUnavailable();

            int getMerchantGold();

            void restock();
    };
}

#endif
