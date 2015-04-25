#include "tradewindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ControllerManager.h>

#include <openengine/misc/rng.hpp>

#include <components/widgets/numericeditbox.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/creaturestats.hpp"

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
        , mCurrentBalance(0)
        , mCurrentMerchantOffer(0)
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
        mTotalBalance->setMinValue(INT_MIN+1); // disallow INT_MIN since abs(INT_MIN) is undefined

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

        mCurrentBalance = 0;
        mCurrentMerchantOffer = 0;

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
            dialog->open(object.getClass().getName(object), message, count);
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

        // were there any items traded at all?
        std::vector<ItemStack> playerBought = playerItemModel->getItemsBorrowedToUs();
        std::vector<ItemStack> merchantBought = mTradeModel->getItemsBorrowedToUs();
        if (playerBought.empty() && merchantBought.empty())
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog11}");
            return;
        }

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        // check if the player can afford this
        if (mCurrentBalance < 0 && playerGold < std::abs(mCurrentBalance))
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog1}");
            return;
        }

        // check if the merchant can afford this
        if (mCurrentBalance > 0 && getMerchantGold() < mCurrentBalance)
        {
            // user notification
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

        // TODO: move to mwmechanics

        // Is the player buying?
        bool buying = (mCurrentMerchantOffer < 0);

        if(mCurrentBalance > mCurrentMerchantOffer)
        {
            //if npc is a creature: reject (no haggle)
            if (mPtr.getTypeName() != typeid(ESM::NPC).name())
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}");
                return;
            }

            int a = abs(mCurrentMerchantOffer);
            int b = abs(mCurrentBalance);
            int d = 0;
            if (buying)
                d = int(100 * (a - b) / a);
            else
                d = int(100 * (b - a) / a);

            int clampedDisposition = std::max(0, std::min(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr)
                + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange(),100));

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
            float x = gmst.find("fBargainOfferMulti")->getFloat() * d + gmst.find("fBargainOfferBase")->getFloat();
            if (buying)
                x += abs(int(pcTerm - npcTerm));
            else
                x += abs(int(npcTerm - pcTerm));

            int roll = OEngine::Misc::Rng::rollDice(100) + 1;
            if(roll > x || (mCurrentMerchantOffer < 0) != (mCurrentBalance < 0)) //trade refused
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}");

                int iBarterFailDisposition = gmst.find("iBarterFailDisposition")->getInt();
                if (mPtr.getClass().isNpc())
                    MWBase::Environment::get().getDialogueManager()->applyDispositionChange(iBarterFailDisposition);
                return;
            }

            //skill use!
            float skillGain = 0.f;
            int finalPrice = std::abs(mCurrentBalance);
            int initialMerchantOffer = std::abs(mCurrentMerchantOffer);
            if (!buying && (finalPrice > initialMerchantOffer) && finalPrice > 0)
                skillGain = floor(100 * (finalPrice - initialMerchantOffer) / float(finalPrice));
            else if (buying && (finalPrice < initialMerchantOffer) && initialMerchantOffer > 0)
                skillGain = floor(100 * (initialMerchantOffer - finalPrice) / float(initialMerchantOffer));

            player.getClass().skillUsageSucceeded(player, ESM::Skill::Mercantile, 0, skillGain);
        }

        int iBarterSuccessDisposition = gmst.find("iBarterSuccessDisposition")->getInt();
        if (mPtr.getClass().isNpc())
            MWBase::Environment::get().getDialogueManager()->applyDispositionChange(iBarterSuccessDisposition);

        // make the item transfer
        mTradeModel->transferItems();
        playerItemModel->transferItems();

        // transfer the gold
        if (mCurrentBalance != 0)
        {
            addOrRemoveGold(mCurrentBalance, player);
            mPtr.getClass().getCreatureStats(mPtr).setGoldPool(
                        mPtr.getClass().getCreatureStats(mPtr).getGoldPool() - mCurrentBalance );
        }

        MWBase::Environment::get().getWindowManager()->getDialogueWindow()->addResponse(
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("sBarterDialog5")->getString());

        std::string sound = "Item Gold Up";
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void TradeWindow::onMaxSaleButtonClicked(MyGUI::Widget* _sender)
    {
        mCurrentBalance = getMerchantGold();
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
        // Entering a "-" sign inverts the buying/selling state
        mCurrentBalance = (mCurrentBalance >= 0 ? 1 : -1) * value;
        updateLabels();

        if (value != std::abs(value))
            mTotalBalance->setValue(std::abs(value));
    }

    void TradeWindow::onIncreaseButtonTriggered()
    {
        // prevent overflows, and prevent entering INT_MIN since abs(INT_MIN) is undefined
        if (mCurrentBalance == INT_MAX || mCurrentBalance == INT_MIN+1)
            return;
        if(mCurrentBalance<=-1) mCurrentBalance -= 1;
        if(mCurrentBalance>=1) mCurrentBalance += 1;
        updateLabels();
    }

    void TradeWindow::onDecreaseButtonTriggered()
    {
        if(mCurrentBalance<-1) mCurrentBalance += 1;
        if(mCurrentBalance>1) mCurrentBalance -= 1;
        updateLabels();
    }

    void TradeWindow::updateLabels()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + MyGUI::utility::toString(playerGold));

        if (mCurrentBalance > 0)
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalSold}");
        }
        else
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalCost}");
        }

        mTotalBalance->setValue(std::abs(mCurrentBalance));

        mMerchantGold->setCaptionWithReplacing("#{sSellerGold} " + MyGUI::utility::toString(getMerchantGold()));
    }

    void TradeWindow::updateOffer()
    {
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

        int diff = merchantOffer - mCurrentMerchantOffer;
        mCurrentMerchantOffer = merchantOffer;
        mCurrentBalance += diff;
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
