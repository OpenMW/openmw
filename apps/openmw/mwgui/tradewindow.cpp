#include "tradewindow.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/manualref.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/player.hpp"

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

        mFilterAll->setStateSelected(true);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onCancelButtonClicked);
        mOfferButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onOfferButtonClicked);
        mIncreaseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onIncreaseButtonClicked);
        mDecreaseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onDecreaseButtonClicked);

        setCoord(400, 0, 400, 300);

        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &TradeWindow::onWindowResize);
    }

    void TradeWindow::startTrade(MWWorld::Ptr actor)
    {
        setTitle(MWWorld::Class::get(actor).getName(actor));

        mCurrentBalance = 0;
        mCurrentMerchantOffer = 0;

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

    void TradeWindow::addOrRemoveGold(int amount)
    {
        bool goldFound = false;
        MWWorld::Ptr gold;
        MWWorld::ContainerStore& playerStore = mWindowManager.getInventoryWindow()->getContainerStore();

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        for (MWWorld::ContainerStoreIterator it = playerStore.begin();
                it != playerStore.end(); ++it)
        {
            if (MWWorld::Class::get(*it).getName(*it) == gmst.find("sGold")->getString())
            {
                goldFound = true;
                gold = *it;
            }
        }
        if (goldFound)
        {
            gold.getRefData().setCount(gold.getRefData().getCount() + amount);
        }
        else
        {
            assert(amount > 0);
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), "Gold_001");
            ref.getPtr().getRefData().setCount(amount);
            playerStore.add(ref.getPtr());
        }
    }

    void TradeWindow::onOfferButtonClicked(MyGUI::Widget* _sender)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        // were there any items traded at all?
        MWWorld::ContainerStore& playerBought = mWindowManager.getInventoryWindow()->getBoughtItems();
        MWWorld::ContainerStore& merchantBought = getBoughtItems();
        if (playerBought.begin() == playerBought.end() && merchantBought.begin() == merchantBought.end())
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog11}", std::vector<std::string>());
            return;
        }

        // check if the player can afford this
        if (mCurrentBalance < 0 && mWindowManager.getInventoryWindow()->getPlayerGold() < std::abs(mCurrentBalance))
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog1}", std::vector<std::string>());
            return;
        }

        // check if the merchant can afford this
        int merchantgold;
        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->mBase->mNpdt52.mGold == -10)
                merchantgold = ref->mBase->mNpdt12.mGold;
            else
                merchantgold = ref->mBase->mNpdt52.mGold;
        }
        else // ESM::Creature
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            merchantgold = ref->mBase->mData.mGold;
        }
        if (mCurrentBalance > 0 && merchantgold < mCurrentBalance)
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog2}", std::vector<std::string>());
            return;
        }

        if(mCurrentBalance > mCurrentMerchantOffer)
        {
            //if npc is a creature: reject (no haggle)
            if (mPtr.getTypeName() != typeid(ESM::NPC).name())
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}", std::vector<std::string>());
                return;
            }

            int a = abs(mCurrentMerchantOffer);
            int b = abs(mCurrentBalance);
            int d = 0;
            if (mCurrentMerchantOffer<0) d = int(100 * (a - b) / a);
            else d = int(100 * (b - a) / a);

            float clampedDisposition = std::max<int>(0,std::min<int>(int(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr)
                + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange()),100));

            MWMechanics::NpcStats sellerSkill = MWWorld::Class::get(mPtr).getNpcStats(mPtr);
            MWMechanics::CreatureStats sellerStats = MWWorld::Class::get(mPtr).getCreatureStats(mPtr);
            MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            MWMechanics::NpcStats playerSkill = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);
            MWMechanics::CreatureStats playerStats = MWWorld::Class::get(playerPtr).getCreatureStats(playerPtr);

            float a1 = std::min(playerSkill.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
            float b1 = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float c1 = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
            float d1 = std::min(sellerSkill.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
            float e1 = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float f1 = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);

            float pcTerm = (clampedDisposition - 50 + a1 + b1 + c1) * playerStats.getFatigueTerm();
            float npcTerm = (d1 + e1 + f1) * sellerStats.getFatigueTerm();
            float x = gmst.find("fBargainOfferMulti")->getFloat() * d + gmst.find("fBargainOfferBase")->getFloat();
            if (mCurrentMerchantOffer<0) x += abs(int(pcTerm - npcTerm));
            else x += abs(int(npcTerm - pcTerm));

            int roll = std::rand()%100 + 1;
            if(roll > x) //trade refused
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}", std::vector<std::string>());

                int iBarterFailDisposition = gmst.find("iBarterFailDisposition")->getInt();
                MWBase::Environment::get().getDialogueManager()->applyTemporaryDispositionChange(iBarterFailDisposition);
                return;
            }

            //skill use!
            MWWorld::Class::get(playerPtr).skillUsageSucceeded(playerPtr, ESM::Skill::Mercantile, 0);
	}

        int iBarterSuccessDisposition = gmst.find("iBarterSuccessDisposition")->getInt();
        MWBase::Environment::get().getDialogueManager()->applyTemporaryDispositionChange(iBarterSuccessDisposition);

        // success! make the item transfer.
        transferBoughtItems();
        mWindowManager.getInventoryWindow()->transferBoughtItems();

        // add or remove gold from the player.
        if (mCurrentBalance != 0)
            addOrRemoveGold(mCurrentBalance);

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

    void TradeWindow::onIncreaseButtonClicked(MyGUI::Widget* _sender)
    {
        if(mCurrentBalance<=-1) mCurrentBalance -= 1;
        if(mCurrentBalance>=1) mCurrentBalance += 1;
        updateLabels();
    }

    void TradeWindow::onDecreaseButtonClicked(MyGUI::Widget* _sender)
    {
        if(mCurrentBalance<-1) mCurrentBalance += 1;
        if(mCurrentBalance>1) mCurrentBalance -= 1;
        updateLabels();
    }

    void TradeWindow::updateLabels()
    {
        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + boost::lexical_cast<std::string>(mWindowManager.getInventoryWindow()->getPlayerGold()));

        if (mCurrentBalance > 0)
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalSold}");
            mTotalBalance->setCaption(boost::lexical_cast<std::string>(mCurrentBalance));
        }
        else
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalCost}");
            mTotalBalance->setCaption(boost::lexical_cast<std::string>(-mCurrentBalance));
        }

        int merchantgold;
        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->mBase->mNpdt52.mGold == -10)
                merchantgold = ref->mBase->mNpdt12.mGold;
            else
                merchantgold = ref->mBase->mNpdt52.mGold;
        }
        else // ESM::Creature
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            merchantgold = ref->mBase->mData.mGold;
        }

        mMerchantGold->setCaptionWithReplacing("#{sSellerGold} " + boost::lexical_cast<std::string>(merchantgold));
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
            if (ref->mBase->mHasAI)
                services = ref->mBase->mAiData.mServices;
        }
        else if (mPtr.getTypeName() == typeid(ESM::Creature).name())
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            if (ref->mBase->mHasAI)
                services = ref->mBase->mAiData.mServices;
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

    void TradeWindow::sellToNpc(MWWorld::Ptr item, int count, bool boughtItem)
    {
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, MWWorld::Class::get(item).getValue(item) * count, boughtItem);

        mCurrentBalance += diff;
        mCurrentMerchantOffer += diff;

        updateLabels();
    }

    void TradeWindow::buyFromNpc(MWWorld::Ptr item, int count, bool soldItem)
    {
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, MWWorld::Class::get(item).getValue(item) * count, !soldItem);

        mCurrentBalance -= diff;
        mCurrentMerchantOffer -= diff;

        updateLabels();
    }

    void TradeWindow::onReferenceUnavailable()
    {
        // remove both Trade and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        mWindowManager.removeGuiMode(GM_Barter);
        mWindowManager.removeGuiMode(GM_Dialogue);
    }
}
