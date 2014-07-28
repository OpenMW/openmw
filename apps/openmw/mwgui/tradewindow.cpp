#include "tradewindow.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "inventorywindow.hpp"
#include "itemview.hpp"
#include "sortfilteritemmodel.hpp"
#include "containeritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
#include "dialogue.hpp"

namespace
{

    int getEffectiveValue (MWWorld::Ptr item, int count)
    {
        int price = item.getClass().getValue(item) * count;
        if (item.getClass().hasItemHealth(item))
            price *= (static_cast<float>(item.getClass().getItemHealth(item)) / item.getClass().getItemMaxHealth(item));
        return price;
    }

}

namespace MWGui
{
    const float TradeWindow::sBalanceChangeInitialPause = 0.5;
    const float TradeWindow::sBalanceChangeInterval = 0.1;

    TradeWindow::TradeWindow()
        : WindowBase("openmw_trade_window.layout")
        , mCurrentBalance(0)
        , mBalanceButtonsState(BBS_None)
        , mBalanceChangePause(0.0)
        , mItemToSell(-1)
        , mTradeModel(NULL)
        , mSortModel(NULL)
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

        mTotalBalance->eventEditTextChange += MyGUI::newDelegate(this, &TradeWindow::onBalanceEdited);

        setCoord(400, 0, 400, 300);
    }

    void TradeWindow::startTrade(const MWWorld::Ptr& actor)
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

        updateLabels();

        // Careful here. setTitle may cause size updates, causing itemview redraw, so make sure to do it last
        // or we end up using a possibly invalid model.
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

        static_cast<MyGUI::Button*>(_sender)->setStateSelected(true);

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

    void TradeWindow::onFrame(float frameDuration)
    {
        if (!mMainWidget->getVisible() || mBalanceButtonsState == BBS_None)
            return;

        mBalanceChangePause -= frameDuration;
        if (mBalanceChangePause < 0.0) {
            mBalanceChangePause += sBalanceChangeInterval;
            if (mBalanceButtonsState == BBS_Increase)
                onIncreaseButtonTriggered();
            else if (mBalanceButtonsState == BBS_Decrease)
                onDecreaseButtonTriggered();
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
            if (Misc::StringUtils::ciEqual(it->mBase.getCellRef().getOwner(), mPtr.getCellRef().getRefId()))
            {
                std::string msg = gmst.find("sNotifyMessage49")->getString();
                if (msg.find("%s") != std::string::npos)
                    msg.replace(msg.find("%s"), 2, it->mBase.getClass().getName(it->mBase));
                MWBase::Environment::get().getWindowManager()->messageBox(msg);
                MWBase::Environment::get().getDialogueManager()->say(mPtr, "Thief");
                MWBase::Environment::get().getMechanicsManager()->reportCrime(player, mPtr, MWBase::MechanicsManager::OT_Theft,
                                                                              it->mBase.getClass().getValue(it->mBase)
                                                                              * it->mCount);
                onCancelButtonClicked(mCancelButton);
                MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Dialogue);
                return;
            }
        }

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

            float clampedDisposition = std::max<int>(0,std::min<int>(int(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr)
                + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange()),100));

            const MWMechanics::CreatureStats &sellerStats = mPtr.getClass().getCreatureStats(mPtr);
            const MWMechanics::CreatureStats &playerStats = player.getClass().getCreatureStats(player);

            float a1 = std::min(player.getClass().getSkill(player, ESM::Skill::Mercantile), 100);
            float b1 = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float c1 = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
            float d1 = std::min(mPtr.getClass().getSkill(mPtr, ESM::Skill::Mercantile), 100);
            float e1 = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float f1 = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);

            float pcTerm = (clampedDisposition - 50 + a1 + b1 + c1) * playerStats.getFatigueTerm();
            float npcTerm = (d1 + e1 + f1) * sellerStats.getFatigueTerm();
            float x = gmst.find("fBargainOfferMulti")->getFloat() * d + gmst.find("fBargainOfferBase")->getFloat();
            if (buying)
                x += abs(int(pcTerm - npcTerm));
            else
                x += abs(int(npcTerm - pcTerm));

            int roll = std::rand()%100 + 1;
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
            player.getClass().skillUsageSucceeded(player, ESM::Skill::Mercantile, 0);
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

    void TradeWindow::onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        mBalanceButtonsState = BBS_Increase;
        mBalanceChangePause = sBalanceChangeInitialPause;
        onIncreaseButtonTriggered();
    }

    void TradeWindow::onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        mBalanceButtonsState = BBS_Decrease;
        mBalanceChangePause = sBalanceChangeInitialPause;
        onDecreaseButtonTriggered();
    }

    void TradeWindow::onBalanceButtonReleased(MyGUI::Widget *_sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        mBalanceButtonsState = BBS_None;
    }

    void TradeWindow::onBalanceEdited(MyGUI::EditBox *_sender)
    {
        try
        {
            unsigned int count = boost::lexical_cast<unsigned int>(_sender->getCaption());
            mCurrentBalance = count * (mCurrentBalance >= 0 ? 1 : -1);
            updateLabels();
        }
        catch (std::bad_cast&)
        {
        }
    }

    void TradeWindow::onIncreaseButtonTriggered()
    {
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

        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + boost::lexical_cast<std::string>(playerGold));

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

        mMerchantGold->setCaptionWithReplacing("#{sSellerGold} " + boost::lexical_cast<std::string>(getMerchantGold()));
    }

    void TradeWindow::sellToNpc(const MWWorld::Ptr& item, int count, bool boughtItem)
    {
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, getEffectiveValue(item, count), boughtItem);

        mCurrentBalance += diff;
        mCurrentMerchantOffer += diff;

        updateLabels();
    }

    void TradeWindow::buyFromNpc(const MWWorld::Ptr& item, int count, bool soldItem)
    {
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, getEffectiveValue(item, count), !soldItem);

        mCurrentBalance -= diff;
        mCurrentMerchantOffer -= diff;

        updateLabels();
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
