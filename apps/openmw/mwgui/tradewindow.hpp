#ifndef MWGUI_TRADEWINDOW_H
#define MWGUI_TRADEWINDOW_H

#include "container.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
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

            void startTrade(const MWWorld::Ptr& actor);

            void onFrame(float frameDuration);

            void borrowItem (int index, size_t count);
            void returnItem (int index, size_t count);

            int getMerchantServices();

            virtual void exit();

            virtual void resetReference();

        private:
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

            MyGUI::Button* mIncreaseButton;
            MyGUI::Button* mDecreaseButton;
            MyGUI::TextBox* mTotalBalanceLabel;
            MyGUI::EditBox* mTotalBalance;

            MyGUI::Widget* mBottomPane;

            MyGUI::Button* mMaxSaleButton;
            MyGUI::Button* mCancelButton;
            MyGUI::Button* mOfferButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mMerchantGold;

            int mItemToSell;

            int mCurrentBalance;
            int mCurrentMerchantOffer;

            enum BalanceButtonsState {
                BBS_None,
                BBS_Increase,
                BBS_Decrease
            } mBalanceButtonsState;
            /// pause before next balance change will trigger while user holds +/- button pressed
            float mBalanceChangePause;

            void sellToNpc(const MWWorld::Ptr& item, int count, bool boughtItem); ///< only used for adjusting the gold balance
            void buyFromNpc(const MWWorld::Ptr& item, int count, bool soldItem); ///< only used for adjusting the gold balance

            void onItemSelected (int index);
            void sellItem (MyGUI::Widget* sender, int count);

            void onFilterChanged(MyGUI::Widget* _sender);
            void onOfferButtonClicked(MyGUI::Widget* _sender);
            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onMaxSaleButtonClicked(MyGUI::Widget* _sender);
            void onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onBalanceButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onBalanceEdited(MyGUI::EditBox* _sender);

            void onIncreaseButtonTriggered();
            void onDecreaseButtonTriggered();

            void addOrRemoveGold(int gold, const MWWorld::Ptr& actor);

            void updateLabels();

            virtual void onReferenceUnavailable();

            int getMerchantGold();
    };
}

#endif
