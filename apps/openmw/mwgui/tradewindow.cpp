#include "tradewindow.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

#include "inventorywindow.hpp"

namespace MWGui
{
    TradeWindow::TradeWindow(MWBase::WindowManager& parWindowManager) :
        WindowBase("openmw_trade_window.layout", parWindowManager)
        , ContainerBase(NULL) // no drag&drop
        , mCurrentBalance(0)
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
        setTitle(MWWorld::Class::get(actor).getName(actor));

        mCurrentBalance = 0;

        mWindowManager.getInventoryWindow()->startTrade();

        mBoughtItems.clear();

        ContainerBase::openContainer(actor);

        updateLabels();

        drawItems();
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
        // were there any items traded at all?
        MWWorld::ContainerStore& playerBought = mWindowManager.getInventoryWindow()->getBoughtItems();
        MWWorld::ContainerStore& merchantBought = getBoughtItems();
        if (playerBought.begin() == playerBought.end() && merchantBought.begin() == merchantBought.end())
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarterDialog11")->str, std::vector<std::string>());
            return;
        }

        // check if the player can afford this
        if (mCurrentBalance < 0 && mWindowManager.getInventoryWindow()->getPlayerGold() < std::abs(mCurrentBalance))
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarterDialog1")->str, std::vector<std::string>());
            return;
        }

        // check if the merchant can afford this
        int merchantgold;
        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->base->npdt52.gold == -10)
                merchantgold = ref->base->npdt12.gold;
            else
                merchantgold = ref->base->npdt52.gold;
        }
        else // ESM::Creature
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            merchantgold = ref->base->data.gold;
        }
        if (mCurrentBalance > 0 && merchantgold < mCurrentBalance)
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sBarterDialog2")->str, std::vector<std::string>());
            return;
        }

        // success! make the item transfer.
        transferBoughtItems();
        mWindowManager.getInventoryWindow()->transferBoughtItems();

        // add or remove gold from the player.
        bool goldFound = false;
        MWWorld::Ptr gold;
        MWWorld::ContainerStore& playerStore = mWindowManager.getInventoryWindow()->getContainerStore();
        for (MWWorld::ContainerStoreIterator it = playerStore.begin();
                it != playerStore.end(); ++it)
        {
            if (MWWorld::Class::get(*it).getName(*it) == MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sGold")->str)
            {
                goldFound = true;
                gold = *it;
            }
        }
        if (goldFound)
        {
            gold.getRefData().setCount(gold.getRefData().getCount() + mCurrentBalance);
        }
        else
        {
            assert(mCurrentBalance > 0);
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), "Gold_001");
            ref.getPtr().getRefData().setCount(mCurrentBalance);
            playerStore.add(ref.getPtr());
        }

        std::string sound = "Item Gold Up";
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        mWindowManager.removeGuiMode(GM_Barter);
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        // i give you back your stuff!
        returnBoughtItems(mWindowManager.getInventoryWindow()->getContainerStore());
        // now gimme back my stuff!
        mWindowManager.getInventoryWindow()->returnBoughtItems(MWWorld::Class::get(mPtr).getContainerStore(mPtr));

        mWindowManager.removeGuiMode(GM_Barter);
    }

    void TradeWindow::updateLabels()
    {
        mPlayerGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sYourGold")->str
            + " " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));

        if (mCurrentBalance > 0)
        {
            mTotalBalanceLabel->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTotalSold")->str);
            mTotalBalance->setCaption(boost::lexical_cast<std::string>(mCurrentBalance));
        }
        else
        {
            mTotalBalanceLabel->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTotalCost")->str);
            mTotalBalance->setCaption(boost::lexical_cast<std::string>(-mCurrentBalance));
        }

        int merchantgold;
        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->base->npdt52.gold == -10)
                merchantgold = ref->base->npdt12.gold;
            else
                merchantgold = ref->base->npdt52.gold;
        }
        else // ESM::Creature
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            merchantgold = ref->base->data.gold;
        }

        mMerchantGold->setCaption(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sSellerGold")->str
            + " " + boost::lexical_cast<std::string>(merchantgold));
    }

    std::vector<MWWorld::Ptr> TradeWindow::getEquippedItems()
    {
        std::vector<MWWorld::Ptr> items;

        if (mPtr.getTypeName() == typeid(ESM::Creature).name())
        {
            // creatures don't have equipment slots.
            return items;
        }

        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);

        for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
            if (it != invStore.end())
            {
                items.push_back(*it);
            }
        }

        return items;
    }

    bool TradeWindow::npcAcceptsItem(MWWorld::Ptr item)
    {
        int services = 0;
        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->base->mHasAI)
                services = ref->base->mAiData.mServices;
        }
        else if (mPtr.getTypeName() == typeid(ESM::Creature).name())
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            if (ref->base->mHasAI)
                services = ref->base->mAiData.mServices;
        }

        /// \todo what about potions, there doesn't seem to be a flag for them??

        if      (item.getTypeName() == typeid(ESM::Weapon).name())
            return services & ESM::NPC::Weapon;
        else if (item.getTypeName() == typeid(ESM::Armor).name())
            return services & ESM::NPC::Armor;
        else if (item.getTypeName() == typeid(ESM::Clothing).name())
            return services & ESM::NPC::Clothing;
        else if (item.getTypeName() == typeid(ESM::Book).name())
            return services & ESM::NPC::Books;
        else if (item.getTypeName() == typeid(ESM::Ingredient).name())
            return services & ESM::NPC::Ingredients;
        else if (item.getTypeName() == typeid(ESM::Tool).name())
            return services & ESM::NPC::Picks;
        else if (item.getTypeName() == typeid(ESM::Probe).name())
            return services & ESM::NPC::Probes;
        else if (item.getTypeName() == typeid(ESM::Light).name())
            return services & ESM::NPC::Lights;
        else if (item.getTypeName() == typeid(ESM::Apparatus).name())
            return services & ESM::NPC::Apparatus;
        else if (item.getTypeName() == typeid(ESM::Repair).name())
            return services & ESM::NPC::RepairItem;
        else if (item.getTypeName() == typeid(ESM::Miscellaneous).name())
            return services & ESM::NPC::Misc;

        return false;
    }

    std::vector<MWWorld::Ptr> TradeWindow::itemsToIgnore()
    {
        std::vector<MWWorld::Ptr> items;
        MWWorld::ContainerStore& invStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

        for (MWWorld::ContainerStoreIterator it = invStore.begin();
                it != invStore.end(); ++it)
        {
            if (!npcAcceptsItem(*it))
                items.push_back(*it);
        }

        return items;
    }

    void TradeWindow::sellToNpc(MWWorld::Ptr item, int count)
    {
        /// \todo price adjustment depending on merchantile skill

        mCurrentBalance -= MWWorld::Class::get(item).getValue(item) * count;

        updateLabels();
    }

    void TradeWindow::buyFromNpc(MWWorld::Ptr item, int count)
    {
        /// \todo price adjustment depending on merchantile skill

        mCurrentBalance += MWWorld::Class::get(item).getValue(item) * count;

        updateLabels();
    }

    void TradeWindow::onReferenceUnavailable()
    {
        // remove both Trade and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        mWindowManager.removeGuiMode(GM_Barter);
        mWindowManager.removeGuiMode(GM_Dialogue);
    }
}
