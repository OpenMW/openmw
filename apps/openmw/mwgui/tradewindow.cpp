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
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/player.hpp"

#include "inventorywindow.hpp"
#include "itemview.hpp"
#include "sortfilteritemmodel.hpp"
#include "containeritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"

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

        setCoord(400, 0, 400, 300);
    }

    void TradeWindow::startTrade(const MWWorld::Ptr& actor)
    {
        mPtr = actor;
        setTitle(MWWorld::Class::get(actor).getName(actor));

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
        return MWWorld::Class::get(mPtr).getServices(mPtr);
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
            dialog->open(MWWorld::Class::get(object).getName(object), message, count);
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
        std::string sound = MWWorld::Class::get(item.mBase).getDownSoundId(item.mBase);
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

    void TradeWindow::addOrRemoveGold(int amount)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::ContainerStore& playerStore = MWWorld::Class::get(player).getContainerStore(player);

        if (amount > 0)
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), "Gold_001", amount);
            playerStore.add(ref.getPtr(), player);
        }
        else
        {
            playerStore.remove("gold_001", - amount, player);
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
        if (!playerBought.size() && !merchantBought.size())
        {
            // user notification
            MWBase::Environment::get().getWindowManager()->
                messageBox("#{sBarterDialog11}");
            return;
        }

        // check if the player can afford this
        if (mCurrentBalance < 0 && MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold() < std::abs(mCurrentBalance))
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
            if (mCurrentBalance<0)
                d = int(100 * (a - b) / a);
            else
                d = int(100 * (b - a) / a);

            float clampedDisposition = std::max<int>(0,std::min<int>(int(MWBase::Environment::get().getMechanicsManager()->getDerivedDisposition(mPtr)
                + MWBase::Environment::get().getDialogueManager()->getTemporaryDispositionChange()),100));

            const MWMechanics::NpcStats &sellerStats = MWWorld::Class::get(mPtr).getNpcStats(mPtr);
            MWWorld::Ptr playerPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            const MWMechanics::NpcStats &playerStats = MWWorld::Class::get(playerPtr).getNpcStats(playerPtr);

            float a1 = std::min(playerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
            float b1 = std::min(0.1f * playerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float c1 = std::min(0.2f * playerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);
            float d1 = std::min(sellerStats.getSkill(ESM::Skill::Mercantile).getModified(), 100.f);
            float e1 = std::min(0.1f * sellerStats.getAttribute(ESM::Attribute::Luck).getModified(), 10.f);
            float f1 = std::min(0.2f * sellerStats.getAttribute(ESM::Attribute::Personality).getModified(), 10.f);

            float pcTerm = (clampedDisposition - 50 + a1 + b1 + c1) * playerStats.getFatigueTerm();
            float npcTerm = (d1 + e1 + f1) * sellerStats.getFatigueTerm();
            float x = gmst.find("fBargainOfferMulti")->getFloat() * d + gmst.find("fBargainOfferBase")->getFloat();
            if (mCurrentBalance<0)
                x += abs(int(pcTerm - npcTerm));
            else
                x += abs(int(npcTerm - pcTerm));

            int roll = std::rand()%100 + 1;
            if(roll > x) //trade refused
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sNotifyMessage9}");

                int iBarterFailDisposition = gmst.find("iBarterFailDisposition")->getInt();
                MWBase::Environment::get().getDialogueManager()->applyTemporaryDispositionChange(iBarterFailDisposition);
                return;
            }

            //skill use!
            MWWorld::Class::get(playerPtr).skillUsageSucceeded(playerPtr, ESM::Skill::Mercantile, 0);
        }

        int iBarterSuccessDisposition = gmst.find("iBarterSuccessDisposition")->getInt();
        MWBase::Environment::get().getDialogueManager()->applyTemporaryDispositionChange(iBarterSuccessDisposition);

        // make the item transfer
        mTradeModel->transferItems();
        playerItemModel->transferItems();

        // add or remove gold from the player.
        if (mCurrentBalance != 0)
            addOrRemoveGold(mCurrentBalance);

        std::string sound = "Item Gold Up";
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
    }

    void TradeWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mTradeModel->abort();
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getTradeModel()->abort();
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Barter);
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
        mPlayerGold->setCaptionWithReplacing("#{sYourGold} " + boost::lexical_cast<std::string>(MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getPlayerGold()));

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
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, MWWorld::Class::get(item).getValue(item) * count, boughtItem);

        mCurrentBalance += diff;
        mCurrentMerchantOffer += diff;

        updateLabels();
    }

    void TradeWindow::buyFromNpc(const MWWorld::Ptr& item, int count, bool soldItem)
    {
        int diff = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, MWWorld::Class::get(item).getValue(item) * count, !soldItem);

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
        int merchantGold;

        if (mPtr.getTypeName() == typeid(ESM::NPC).name())
        {
            MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
            if (ref->mBase->mNpdt52.mGold == -10)
                merchantGold = ref->mBase->mNpdt12.mGold;
            else
                merchantGold = ref->mBase->mNpdt52.mGold;
        }
        else // ESM::Creature
        {
            MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
            merchantGold = ref->mBase->mData.mGold;
        }

        return merchantGold;
    }
}
