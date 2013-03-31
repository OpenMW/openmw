#ifndef MWGUI_TRADEWINDOW_H
#define MWGUI_TRADEWINDOW_H

#include "container.hpp"
#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

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
    class TradeWindow : public ContainerBase, public WindowBase
    {
        public:
            TradeWindow(MWBase::WindowManager& parWindowManager);

            void startTrade(MWWorld::Ptr actor);

            void sellToNpc(MWWorld::Ptr item, int count, bool boughtItem); ///< only used for adjusting the gold balance
            void buyFromNpc(MWWorld::Ptr item, int count, bool soldItem); ///< only used for adjusting the gold balance

            bool npcAcceptsItem(MWWorld::Ptr item);

            void addOrRemoveGold(int gold);

            void onFrame(float frameDuration);

        protected:
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
            MyGUI::TextBox* mTotalBalance;

            MyGUI::Widget* mBottomPane;

            MyGUI::Button* mMaxSaleButton;
            MyGUI::Button* mCancelButton;
            MyGUI::Button* mOfferButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mMerchantGold;

            int mCurrentBalance;
            int mCurrentMerchantOffer;

            enum BalanceButtonsState {
                BBS_None,
                BBS_Increase,
                BBS_Decrease
            } mBalanceButtonsState;
            /// pause before next balance change will trigger while user holds +/- button pressed
            float mBalanceChangePause;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
            void onOfferButtonClicked(MyGUI::Widget* _sender);
            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onMaxSaleButtonClicked(MyGUI::Widget* _sender);
            void onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
            void onBalanceButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);

            void onIncreaseButtonTriggered();
            void onDecreaseButtonTriggered();

            virtual bool isTrading() { return true; }
            virtual bool isTradeWindow() { return true; }

            virtual std::vector<MWWorld::Ptr> itemsToIgnore();

            void updateLabels();

            virtual void onReferenceUnavailable();

        private:
            int getMerchantGold();
    };
}

#endif
