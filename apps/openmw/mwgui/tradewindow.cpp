#include "tradewindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/world.hpp"

#include "window_manager.hpp"

namespace MWGui
{
    TradeWindow::TradeWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_trade_window_layout.xml", parWindowManager),
        ContainerBase(NULL) // no drag&drop
    {
        MyGUI::ScrollView* itemView;
        MyGUI::Widget* containerWidget;
        getWidget(containerWidget, "Items");
        getWidget(itemView, "ItemView");
        setWidgets(containerWidget, itemView);

        getWidget(mFilterAll, "AllButton");
        getWidget(mFilterWeapon, "WeaponButton");
        getWidget(mFilterApparel, "ApparelButton");
        getWidget(mFilterMagic, "MagicButton");
        getWidget(mFilterMisc, "MiscButton");

        getWidget(mMaxSaleButton, "MaxSaleButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mOfferButton, "OfferButton");
        getWidget(mPlayerGold, "PlayerGold");
        getWidget(mMerchantGold, "MerchantGold");
        getWidget(mIncreaseButton, "IncreaseButton");
        getWidget(mDecreaseButton, "DecreaseButton");
        getWidget(mTotalBalance, "TotalBalance");
        getWidget(mTotalBalanceLabel, "TotalBalanceLabel");
        getWidget(mBottomPane, "BottomPane");

        // this GMST doesn't seem to get retrieved - even though i can clearly see it in the CS !??!?
        mMaxSaleButton->setCaption(mWindowManager.getGameSettingString("sMaxSale", "Max. Sale"));

        mCancelButton->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sCancel")->str);
        mOfferButton->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sOffer")->str);
/*
        mTotalBalanceLabel->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTotalCost")->str);
        mTotalBalanceLabel->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTotalSold")->str);
        mPlayerGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sYourGold")->str);
        mMerchantGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sSellerGold")->str);

*/
        mFilterAll->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sAllTab")->str);
        mFilterWeapon->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sWeaponTab")->str);
        mFilterApparel->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sApparelTab")->str);
        mFilterMagic->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sMagicTab")->str);
        mFilterMisc->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sMiscTab")->str);

        // adjust size of buttons to fit text
        int curX = 0;
        mFilterAll->setSize( mFilterAll->getTextSize().width + 24, mFilterAll->getSize().height );
        curX += mFilterAll->getTextSize().width + 24 + 4;

        mFilterWeapon->setPosition(curX, mFilterWeapon->getPosition().top);
        mFilterWeapon->setSize( mFilterWeapon->getTextSize().width + 24, mFilterWeapon->getSize().height );
        curX += mFilterWeapon->getTextSize().width + 24 + 4;

        mFilterApparel->setPosition(curX, mFilterApparel->getPosition().top);
        mFilterApparel->setSize( mFilterApparel->getTextSize().width + 24, mFilterApparel->getSize().height );
        curX += mFilterApparel->getTextSize().width + 24 + 4;

        mFilterMagic->setPosition(curX, mFilterMagic->getPosition().top);
        mFilterMagic->setSize( mFilterMagic->getTextSize().width + 24, mFilterMagic->getSize().height );
        curX += mFilterMagic->getTextSize().width + 24 + 4;

        mFilterMisc->setPosition(curX, mFilterMisc->getPosition().top);
        mFilterMisc->setSize( mFilterMisc->getTextSize().width + 24, mFilterMisc->getSize().height );

        mFilterAll->setStateSelected(true);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onCancelButtonClicked);
        mOfferButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onOfferButtonClicked);

        mMaxSaleButton->setSize(MyGUI::IntSize(mMaxSaleButton->getTextSize().width + 24, mMaxSaleButton->getHeight()));

        int cancelButtonWidth = mCancelButton->getTextSize().width + 24;
        mCancelButton->setCoord(mBottomPane->getWidth()-cancelButtonWidth,
                                mCancelButton->getTop(),
                                cancelButtonWidth,
                                mCancelButton->getHeight());

        int offerButtonWidth = mOfferButton->getTextSize().width + 24;
        mOfferButton->setCoord(mBottomPane->getWidth()-cancelButtonWidth-offerButtonWidth-8,
                                mOfferButton->getTop(),
                                offerButtonWidth,
                                mOfferButton->getHeight());

        setCoord(400, 0, 400, 300); 

        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &TradeWindow::onWindowResize);
    }

    void TradeWindow::startTrade(MWWorld::Ptr actor)
    {
        ContainerBase::openContainer(actor);

        setTitle(MWWorld::Class::get(actor).getName(actor));
        adjustWindowCaption();
    }

    void TradeWindow::onFilterChanged(MyGUI::Widget* _sender)
    {
        if (_sender == mFilterAll)
            setFilter(ContainerBase::Filter_All);
        else if (_sender == mFilterWeapon)
            setFilter(ContainerBase::Filter_Weapon);
        else if (_sender == mFilterApparel)
            setFilter(ContainerBase::Filter_Apparel);
        else if (_sender == mFilterMagic)
            setFilter(ContainerBase::Filter_Magic);
        else if (_sender == mFilterMisc)
            setFilter(ContainerBase::Filter_Misc);

        mFilterAll->setStateSelected(false);
        mFilterWeapon->setStateSelected(false);
        mFilterApparel->setStateSelected(false);
        mFilterMagic->setStateSelected(false);
        mFilterMisc->setStateSelected(false);

        static_cast<MyGUI::Button*>(_sender)->setStateSelected(true);
    }

    void TradeWindow::onWindowResize(MyGUI::Window* _sender)
    {
        drawItems();
    }

    void TradeWindow::onOfferButtonClicked(MyGUI::Widget* _sender)
    {
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.setGuiMode(GM_Game);
    }
}
