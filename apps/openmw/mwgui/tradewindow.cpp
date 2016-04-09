#include "tradewindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ControllerManager.h>

#include <components/misc/rng.hpp>

#include <components/widgets/numericeditbox.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "inventorywindow.hpp"
#include "itemview.hpp"
#include "sortfilteritemmodel.hpp"
#include "containeritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
#include "dialogue.hpp"
#include "controllers.hpp"

namespace
{

    int getEffectiveValue (MWWorld::Ptr item, int count)
    {
        float price = static_cast<float>(item.getClass().getValue(item));
        if (item.getClass().hasItemHealth(item))
        {
            price *= item.getClass().getItemHealth(item);
            price /= item.getClass().getItemMaxHealth(item);
        }
        return static_cast<int>(price * count);
    }

}

namespace MWGui
{
    const float TradeWindow::sBalanceChangeInitialPause = 0.5f;
    const float TradeWindow::sBalanceChangeInterval = 0.1f;

    TradeWindow::TradeWindow()
        : WindowBase("openmw_trade_window.layout")
        , mSortModel(NULL)
        , mTradeModel(NULL)
        , mItemToSell(-1)
        , mIsPlayerSelling(false)
        , mIsMerchantBuying(false)
        , mPlayerOffer(0)
        , mMerchantOffer(0)
    {
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

        getWidget(mItemView, "ItemView");
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &TradeWindow::onItemSelected);

        mFilterAll->setStateSelected(true);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onFilterChanged);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onCancelButtonClicked);
        mOfferButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onOfferButtonClicked);
        mMaxSaleButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TradeWindow::onMaxSaleButtonClicked);
        mIncreaseButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &TradeWindow::onIncreaseButtonPressed);
        mIncreaseButton->eventMouseButtonReleased += MyGUI::newDelegate(this, &TradeWindow::onBalanceButtonReleased);
        mDecreaseButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &TradeWindow::onDecreaseButtonPressed);
        mDecreaseButton->eventMouseButtonReleased += MyGUI::newDelegate(this, &TradeWindow::onBalanceButtonReleased);

        mTotalBalance->eventValueChanged += MyGUI::newDelegate(this, &TradeWindow::onBalanceValueChanged);

        setCoord(400, 0, 400, 300);
    }

    void TradeWindow::restock()
    {
        // Restock items on the actor inventory
        mPtr.getClass().restock(mPtr);

        // Also restock any containers owned by this merchant, which are also available to buy in the trade window
        std::vector<MWWorld::Ptr> itemSources;
        MWBase::Environment::get().getWorld()->getContainersOwnedBy(mPtr, itemSources);
        for (std::vector<MWWorld::Ptr>::iterator it = itemSources.begin(); it != itemSources.end(); ++it)
        {
            it->getClass().restock(*it);
        }
    }

    void TradeWindow::startTrade(const MWWorld::Ptr& actor)
    {
        mPtr = actor;

        mIsPlayerSelling = false;
        mIsMerchantBuying = false;
        mPlayerOffer = 0;
        mMerchantOffer = 0;

        restock();

        std::vector<MWWorld::Ptr> itemSources;
        MWBase::Environment::get().getWorld()->getContainersOwnedBy(actor, itemSources);

        // Important: actor goes last, so that items purchased by the merchant go into his inventory
        itemSources.push_back(actor);
        std::vector<MWWorld::Ptr> worldItems;
        MWBase::Environment::get().getWorld()->getItemsOwnedBy(actor, worldItems);

        mTradeModel = new TradeItemModel(new ContainerItemModel(itemSources, worldItems), mPtr);
        mSortModel = new SortFilterItemModel(mTradeModel);
        mItemView->setModel (mSortModel);
        mItemView->resetScrollBars();

        updateLabels();

        setTitle(actor.getClass().getName(actor));

        onFilterChanged(mFilterAll);
    }

    void TradeWindow::onFilterChanged(MyGUI::Widget* _sender)
    {
        if (_sender == mFilterAll)
            mSortModel->setCategory(SortFilterItemModel::Category_All);
        else if (_sender == mFilterWeapon)
            mSortModel->setCategory(SortFilterItemModel::Category_Weapon);
        else if (_sender == mFilterApparel)
            mSortModel->setCategory(SortFilterItemModel::Category_Apparel);
        else if (_sender == mFilterMagic)
            mSortModel->setCategory(SortFilterItemModel::Category_Magic);
        else if (_sender == mFilterMisc)
            mSortModel->setCategory(SortFilterItemModel::Category_Misc);

        mFilterAll->setStateSelected(false);
        mFilterWeapon->setStateSelected(false);
        mFilterApparel->setStateSelected(false);
        mFilterMagic->setStateSelected(false);
        mFilterMisc->setStateSelected(false);

        _sender->castType<MyGUI::Button>()->setStateSelected(true);

        mItemView->update();
    }

    int TradeWindow::getMerchantServices()
    {
        return mPtr.getClass().getServices(mPtr);
    }

    void TradeWindow::exit()
    {
        mTradeModel->abort();
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel()->abort();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
    }

    void TradeWindow::onItemSelected (int index)
    {
        const ItemStack& item = mSortModel->getItem(index);

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;
        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string message = "#{sQuanityMenuMessage02}";
            dialog->openCountDialog(object.getClass().getName(object), message, count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &TradeWindow::sellItem);
            mItemToSell = mSortModel->mapToSource(index);
        }
        else
        {
            mItemToSell = mSortModel->mapToSource(index);
            sellItem (NULL, count);
        }
    }

    void TradeWindow::sellItem(MyGUI::Widget* sender, int count)
    {
        const ItemStack& item = mTradeModel->getItem(mItemToSell);
        std::string sound = item.mBase.getClass().getDownSoundId(item.mBase);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        TradeItemModel* playerTradeModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel();

        if (item.mType == ItemStack::Type_Barter)
        {
            // this was an item borrowed to us by the player
            mTradeModel->returnItemBorrowedToUs(mItemToSell, count);
            playerTradeModel->returnItemBorrowedFromUs(mItemToSell, mTradeModel, count);
            buyFromNpc(item.mBase, count, true);
        }
        else
        {
            // borrow item to player
            playerTradeModel->borrowItemToUs(mItemToSell, mTradeModel, count);
            mTradeModel->borrowItemFromUs(mItemToSell, count);
            buyFromNpc(item.mBase, count, false);
        }

        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
        mItemView->update();
    }

    void TradeWindow::borrowItem (int index, size_t count)
    {
        TradeItemModel* playerTradeModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel();
        mTradeModel->borrowItemToUs(index, playerTradeModel, count);
        mItemView->update();
        sellToNpc(playerTradeModel->getItem(index).mBase, count, false);
    }

    void TradeWindow::returnItem (int index, size_t count)
    {
        TradeItemModel* playerTradeModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel();
        const ItemStack& item = playerTradeModel->getItem(index);
        mTradeModel->returnItemBorrowedFromUs(index, playerTradeModel, count);
        mItemView->update();
        sellToNpc(item.mBase, count, true);
    }

    void TradeWindow::addOrRemoveGold(int amount, const MWWorld::Ptr& actor)
    {
        MWWorld::ContainerStore& store = actor.getClass().getContainerStore(actor);

        if (amount > 0)
        {
            store.add(MWWorld::ContainerStore::sGoldId, amount, actor);
        }
        else
        {
            store.remove(MWWorld::ContainerStore::sGoldId, - amount, actor);
        }
    }

    void TradeWindow::onOfferButtonClicked(MyGUI::Widget* _sender)
    {
        TradeItemModel* playerItemModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel();

        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        // notify if no items were traded
        std::vector<ItemStack> playerBought = playerItemModel->getItemsBorrowedToUs();
        std::vector<ItemStack> merchantBought = mTradeModel->getItemsBorrowedToUs();

        if ( playerBought.empty() && merchantBought.empty() ) {
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog11}");
            return;
        }

        // notify if the player cannot afford to buy
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if ( !mIsPlayerSelling && playerGold < mPlayerOffer ) {
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog1}");
            return;
        }

        // notify if the merchant cannot afford to buy
        if ( mIsPlayerSelling && getMerchantGold() < mPlayerOffer ) {
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog2}");
            return;
        }

        // check if the player is attempting to sell back an item stolen from this actor
        for (std::vector<ItemStack>::iterator it = merchantBought.begin(); it != merchantBought.end(); ++it)
        {
            if (MWBase::Environment::get().getMechanicsManager()->isItemStolenFrom(it->mBase.getCellRef().getRefId(),
                                                                                   mPtr.getCellRef().getRefId()))
            {
                std::string msg = gmst.find("sNotifyMessage49")->getString();
                if (msg.find("%s") != std::string::npos)
                    msg.replace(msg.find("%s"), 2, it->mBase.getClass().getName(it->mBase));
                MWBase::Environment::get().getWindowManager()->messageBox(msg);
                MWBase::Environment::get().getMechanicsManager()->commitCrime(player, mPtr, MWBase::MechanicsManager::OT_Theft,
                                                                              it->mBase.getClass().getValue(it->mBase)
                                                                              * it->mCount, true);
                onCancelButtonClicked(mCancelButton);
                MWBase::Environment::get().getDialogueManager()->goodbyeSelected();
                return;
            }
        }

        // TODO: move haggling code to mwmechanics

        // haggle if player's offer is better for player than merchant's offer
        int playerOffer = mPlayerOffer * (mIsPlayerSelling ? 1 : -1);
        int merchantOffer = mMerchantOffer * (mIsMerchantBuying ? 1 : -1);

        if ( playerOffer > merchantOffer ) {
            // don't haggle if npc is a creature
            if ( mPtr.getTypeName() != typeid(ESM::NPC).name() ) {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}");
                return;
            }

            // note: haggle formula is invalid if player tries to buy items and receive gold
            int m = mMerchantOffer;
            int p = mPlayerOffer;
            int percentDiff = int(100.f * std::abs(p - m) / m);

            int clampedDisposition = MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr);

            const MWMechanics::CreatureStats &sellerStats = mPtr.getClass().getCreatureStats(mPtr);
            const MWMechanics::CreatureStats &playerStats = player.getClass().getCreatureStats(player);

            float a1 = static_cast<float>(player.getClass().getSkill(player, ESM::Skill::Mercantile));
            float b1 = 0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified();
            float c1 = 0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified();
            float d1 = static_cast<float>(mPtr.getClass().getSkill(mPtr, ESM::Skill::Mercantile));
            float e1 = 0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified();
            float f1 = 0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified();

            float dispositionTerm = gmst.find("fDispositionMod")->getFloat() * (clampedDisposition - 50);
            float pcTerm = (dispositionTerm - 50 + a1 + b1 + c1) * playerStats.getFatigueTerm();
            float npcTerm = (d1 + e1 + f1) * sellerStats.getFatigueTerm();
            float x = gmst.find("fBargainOfferMulti")->getFloat() * percentDiff
                    + gmst.find("fBargainOfferBase")->getFloat()
                    + std::abs(int(npcTerm - pcTerm));

            // notify and decrease merchant disposition on haggle failure
            // note: haggle auto-fails if player tries to buy items and receive gold (see above)
            int roll = Misc::Rng::rollDice(100) + 1;

            if ( x < roll || (mIsPlayerSelling && !mIsMerchantBuying) ) {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}");

                int iBarterFailDisposition = gmst.find("iBarterFailDisposition")->getInt();
                if (mPtr.getClass().isNpc())
                    MWBase::Environment::get().getDialogueManager()->applyDispositionChange(iBarterFailDisposition);
                return;
            }

            // apply skill gain on haggle success
            float skillGain = 0.f;

            if ( mIsPlayerSelling && p > m && p != 0 ) {
                skillGain = std::floor(100.f * (p - m) / p);
            }
            else if ( !mIsPlayerSelling && p < m && m != 0 ) {
                skillGain = std::floor(100.f * (m - p) / m);
            }

            player.getClass().skillUsageSucceeded(player, ESM::Skill::Mercantile, 0, skillGain);
        }

        // increase merchant's disposition
        int iBarterSuccessDisposition = gmst.find("iBarterSuccessDisposition")->getInt();
        if (mPtr.getClass().isNpc())
            MWBase::Environment::get().getDialogueManager()->applyDispositionChange(iBarterSuccessDisposition);

        // transfer the items
        mTradeModel->transferItems();
        playerItemModel->transferItems();

        // transfer the gold
        addOrRemoveGold(playerOffer, player);
        mPtr.getClass().getCreatureStats(mPtr).setGoldPool(
                    mPtr.getClass().getCreatureStats(mPtr).getGoldPool() - playerOffer);

        // print barter response to dialogue
        MWBase::Environment::get().getWindowManager()->getDialogueWindow()->addResponse(
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sBarterDialog5")->getString());

        // play barter sound
        std::string sound = "Item Gold Up";
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        // exit trade window
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void TradeWindow::onMaxSaleButtonClicked(MyGUI::Widget* _sender)
    {
        mIsPlayerSelling = true;
        mPlayerOffer = getMerchantGold();
        updateLabels();
    }

    void TradeWindow::addRepeatController(MyGUI::Widget *widget)
    {
        MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(Controllers::ControllerRepeatEvent::getClassTypeName());
        Controllers::ControllerRepeatEvent* controller = item->castType<Controllers::ControllerRepeatEvent>();
        controller->eventRepeatClick += MyGUI::newDelegate(this, &TradeWindow::onRepeatClick);
        controller->setRepeat(sBalanceChangeInitialPause, sBalanceChangeInterval);
        MyGUI::ControllerManager::getInstance().addItem(widget, controller);
    }

    void TradeWindow::onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        addRepeatController(_sender);
        onIncreaseButtonTriggered();
    }

    void TradeWindow::onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        addRepeatController(_sender);
        onDecreaseButtonTriggered();
    }

    void TradeWindow::onRepeatClick(MyGUI::Widget* widget, MyGUI::ControllerItem* controller)
    {
        if (widget == mIncreaseButton)
            onIncreaseButtonTriggered();
        else if (widget == mDecreaseButton)
            onDecreaseButtonTriggered();
    }

    void TradeWindow::onBalanceButtonReleased(MyGUI::Widget *_sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        MyGUI::ControllerManager::getInstance().removeItem(_sender);
    }

    void TradeWindow::onBalanceValueChanged(int value)
    {
        // a negative value inverts the buying/selling state
        if ( value < 0 ) {
            mIsPlayerSelling = !mIsPlayerSelling;
        }

        mPlayerOffer = std::abs(value);
        updateLabels();
    }

    void TradeWindow::onIncreaseButtonTriggered()
    {
        if ( mPlayerOffer < INT_MAX ) {
            mPlayerOffer++;
            updateLabels();
        }
    }

    void TradeWindow::onDecreaseButtonTriggered()
    {
        if ( mPlayerOffer > 0 ) {
            mPlayerOffer--;
            updateLabels();
        }
    }

    void TradeWindow::updateLabels()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + MyGUI::utility::toString(playerGold));
        mMerchantGold->setCaptionWithReplacing("#{sSellerGold} " + MyGUI::utility::toString(getMerchantGold()));

        if ( mIsPlayerSelling ) {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalSold}");
        }
        else {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalCost}");
        }
        mTotalBalance->setValue(mPlayerOffer);
    }

    void TradeWindow::updateOffer()
    {
        // get the merchant's offer for all traded items
        TradeItemModel* playerTradeModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel();

        int merchantOffer = 0;

        std::vector<ItemStack> playerBorrowed = playerTradeModel->getItemsBorrowedToUs();
        for (std::vector<ItemStack>::const_iterator it = playerBorrowed.begin(); it != playerBorrowed.end(); ++it)
        {
            merchantOffer -= MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, getEffectiveValue(it->mBase, it->mCount), true);
        }

        std::vector<ItemStack> merchantBorrowed = mTradeModel->getItemsBorrowedToUs();
        for (std::vector<ItemStack>::const_iterator it = merchantBorrowed.begin(); it != merchantBorrowed.end(); ++it)
        {
            merchantOffer += MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, getEffectiveValue(it->mBase, it->mCount), false);
        }

        // apply the merchant's new offer
        mIsPlayerSelling = mIsMerchantBuying = (merchantOffer > 0);
        mPlayerOffer = mMerchantOffer = std::abs(merchantOffer);

        updateLabels();
    }

    void TradeWindow::sellToNpc(const MWWorld::Ptr& item, int count, bool boughtItem)
    {
        updateOffer();
    }

    void TradeWindow::buyFromNpc(const MWWorld::Ptr& item, int count, bool soldItem)
    {
        updateOffer();
    }

    void TradeWindow::onReferenceUnavailable()
    {
        // remove both Trade and Dialogue (since you always trade with the NPC/creature that you have previously talked to)
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
    }

    int TradeWindow::getMerchantGold()
    {
        int merchantGold = mPtr.getClass().getCreatureStats(mPtr).getGoldPool();
        return merchantGold;
    }

    void TradeWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(NULL);
        mTradeModel = NULL;
        mSortModel = NULL;
    }
}
