#include "tradewindow.hpp"

#include <climits>

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_ControllerManager.h>

#include <components/widgets/numericeditbox.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "inventorywindow.hpp"
#include "itemview.hpp"
#include "sortfilteritemmodel.hpp"
#include "containeritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
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
        mTotalBalance->eventEditSelectAccept += MyGUI::newDelegate(this, &TradeWindow::onAccept);
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

    void TradeWindow::setPtr(const MWWorld::Ptr& actor)
    {
        mPtr = actor;

        mCurrentBalance = 0;
        mCurrentMerchantOffer = 0;

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

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTotalBalance);
    }

    void TradeWindow::onFrame(float dt)
    {
        checkReferenceAvailable();
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

    bool TradeWindow::exit()
    {
        mTradeModel->abort();
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel()->abort();
        return true;
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
        std::string sound = item.mBase.getClass().getUpSoundId(item.mBase);
        MWBase::Environment::get().getWindowManager()->playSound(sound);

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

        MWWorld::Ptr player = MWMechanics::getPlayer();
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
            if (MWBase::Environment::get().getMechanicsManager()->isItemStolenFrom(it->mBase.getCellRef().getRefId(), mPtr))
            {
                std::string msg = gmst.find("sNotifyMessage49")->mValue.getString();
                if (msg.find("%s") != std::string::npos)
                    msg.replace(msg.find("%s"), 2, it->mBase.getClass().getName(it->mBase));
                MWBase::Environment::get().getWindowManager()->messageBox(msg);

                MWBase::Environment::get().getMechanicsManager()->confiscateStolenItemToOwner(player, it->mBase, mPtr, it->mCount);

                onCancelButtonClicked(mCancelButton);
                MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
                return;
            }
        }

        bool offerAccepted = mTrading.haggle(player, mPtr, mCurrentBalance, mCurrentMerchantOffer);

        // apply disposition change if merchant is NPC
        if ( mPtr.getClass().isNpc() ) {
            int dispositionDelta = offerAccepted
                ? gmst.find("iBarterSuccessDisposition")->mValue.getInteger()
                : gmst.find("iBarterFailDisposition")->mValue.getInteger();

            MWBase::Environment::get().getDialogueManager()->applyBarterDispositionChange(dispositionDelta);
        }

        // display message on haggle failure
        if ( !offerAccepted ) {
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sNotifyMessage9}");
            return;
        }

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

        eventTradeDone();

        MWBase::Environment::get().getWindowManager()->playSound("Item Gold Up");
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);

        restock();
    }

    void TradeWindow::onAccept(MyGUI::EditBox *sender)
    {
        onOfferButtonClicked(sender);
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
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
        if (mCurrentBalance < 0) mCurrentBalance -= 1;
        else mCurrentBalance += 1;
        updateLabels();
    }

    void TradeWindow::onDecreaseButtonTriggered()
    {
        if (mCurrentBalance < 0) mCurrentBalance += 1;
        else mCurrentBalance -= 1;
        updateLabels();
    }

    void TradeWindow::updateLabels()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + MyGUI::utility::toString(playerGold));

        if (mCurrentBalance < 0)
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalCost}");
        }
        else
        {
            mTotalBalanceLabel->setCaptionWithReplacing("#{sTotalSold}");
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
        MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
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
