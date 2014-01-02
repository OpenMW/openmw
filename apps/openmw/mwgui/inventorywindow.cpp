#include "inventorywindow.hpp"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"

#include "bookwindow.hpp"
#include "scrollwindow.hpp"
#include "spellwindow.hpp"
#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "container.hpp"

namespace MWGui
{

    InventoryWindow::InventoryWindow(DragAndDrop* dragAndDrop)
        : WindowPinnableBase("openmw_inventory_window.layout")
        , mTrading(false)
        , mLastXSize(0)
        , mLastYSize(0)
        , mPreview(MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ())
        , mPreviewDirty(true)
        , mDragAndDrop(dragAndDrop)
        , mSelectedItem(-1)
        , mGuiMode(GM_Inventory)
    {
        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &InventoryWindow::onWindowResize);

        getWidget(mAvatar, "Avatar");
        getWidget(mAvatarImage, "AvatarImage");
        getWidget(mEncumbranceBar, "EncumbranceBar");
        getWidget(mFilterAll, "AllButton");
        getWidget(mFilterWeapon, "WeaponButton");
        getWidget(mFilterApparel, "ApparelButton");
        getWidget(mFilterMagic, "MagicButton");
        getWidget(mFilterMisc, "MiscButton");
        getWidget(mLeftPane, "LeftPane");
        getWidget(mRightPane, "RightPane");
        getWidget(mArmorRating, "ArmorRating");

        mAvatar->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onAvatarClicked);

        getWidget(mItemView, "ItemView");
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &InventoryWindow::onItemSelected);
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &InventoryWindow::onBackgroundSelected);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);

        mFilterAll->setStateSelected(true);

        setGuiMode(mGuiMode);

        adjustPanes();
    }

    void InventoryWindow::adjustPanes()
    {
        const float aspect = 0.5; // fixed aspect ratio for the left pane
        mLeftPane->setSize( (mMainWidget->getSize().height-44) * aspect, mMainWidget->getSize().height-44 );
        mRightPane->setCoord( mLeftPane->getPosition().left + (mMainWidget->getSize().height-44) * aspect + 4,
                              mRightPane->getPosition().top,
                              mMainWidget->getSize().width - 12 - (mMainWidget->getSize().height-44) * aspect - 15,
                              mMainWidget->getSize().height-44 );
    }

    void InventoryWindow::updatePlayer()
    {
        mPtr = MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ();
        mTradeModel = new TradeItemModel(new InventoryItemModel(mPtr), MWWorld::Ptr());
        mSortModel = new SortFilterItemModel(mTradeModel);
        mItemView->setModel(mSortModel);
        mPreview = MWRender::InventoryPreview(mPtr);
        mPreview.setup();
    }

    void InventoryWindow::setGuiMode(GuiMode mode)
    {
        std::string setting = "inventory";
        mGuiMode = mode;
        switch(mode) {
            case GM_Container:
                setPinButtonVisible(false);
                setting += " container";
                break;
            case GM_Companion:
                setPinButtonVisible(false);
                setting += " companion";
                break;
            case GM_Barter:
                setPinButtonVisible(false);
                setting += " barter";
                break;
            case GM_Inventory:
            default:
                setPinButtonVisible(true);
                break;
        }

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos (Settings::Manager::getFloat(setting + " x", "Windows") * viewSize.width,
                             Settings::Manager::getFloat(setting + " y", "Windows") * viewSize.height);
        MyGUI::IntSize size (Settings::Manager::getFloat(setting + " w", "Windows") * viewSize.width,
                             Settings::Manager::getFloat(setting + " h", "Windows") * viewSize.height);

        if (size.width != mMainWidget->getWidth() || size.height != mMainWidget->getHeight())
            mPreviewDirty = true;

        mMainWidget->setPosition(pos);
        mMainWidget->setSize(size);
        adjustPanes();
    }

    TradeItemModel* InventoryWindow::getTradeModel()
    {
        return mTradeModel;
    }

    ItemModel* InventoryWindow::getModel()
    {
        return mTradeModel;
    }

    void InventoryWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
            mDragAndDrop->drop(mTradeModel, mItemView);
    }

    void InventoryWindow::onItemSelected (int index)
    {
        onItemSelectedFromSourceModel (mSortModel->mapToSource(index));
    }

    void InventoryWindow::onItemSelectedFromSourceModel (int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->drop(mTradeModel, mItemView);
            return;
        }

        const ItemStack& item = mTradeModel->getItem(index);

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;

        // Bound items may not be moved
        if (item.mBase.getCellRef().mRefID.size() > 6
                && item.mBase.getCellRef().mRefID.substr(0,6) == "bound_")
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sBarterDialog12}");
            return;
        }

        if (item.mType == ItemStack::Type_Equipped)
        {
            MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
            MWWorld::Ptr newStack = *invStore.unequipItem(item.mBase, mPtr);

            // The unequipped item was re-stacked. We have to update the index
            // since the item pointed does not exist anymore.
            if (item.mBase != newStack)
            {
                // newIndex will store the index of the ItemStack the item was stacked on
                int newIndex = -1;
                for (size_t i=0; i < mTradeModel->getItemCount(); ++i)
                {
                    if (mTradeModel->getItem(i).mBase == newStack)
                    {
                        newIndex = i;
                        break;
                    }
                }

                if (newIndex == -1)
                    throw std::runtime_error("Can't find restacked item");

                index = newIndex;
                object = mTradeModel->getItem(index).mBase;
            }

        }

        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        if (mTrading)
        {
            // check if merchant accepts item
            int services = MWBase::Environment::get().getWindowManager()->getTradeWindow()->getMerchantServices();
            if (!MWWorld::Class::get(object).canSell(object, services))
            {
                MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sBarterDialog4}");
                return;
            }
        }

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string message = mTrading ? "#{sQuanityMenuMessage01}" : "#{sTake}";
            dialog->open(MWWorld::Class::get(object).getName(object), message, count);
            dialog->eventOkClicked.clear();
            if (mTrading)
                dialog->eventOkClicked += MyGUI::newDelegate(this, &InventoryWindow::sellItem);
            else
                dialog->eventOkClicked += MyGUI::newDelegate(this, &InventoryWindow::dragItem);
            mSelectedItem = index;
        }
        else
        {
            mSelectedItem = index;
            if (mTrading)
                sellItem (NULL, count);
            else
                dragItem (NULL, count);
        }

        // item might have been unequipped
        notifyContentChanged();
    }

    void InventoryWindow::dragItem(MyGUI::Widget* sender, int count)
    {
        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mTradeModel, mItemView, count);
    }

    void InventoryWindow::sellItem(MyGUI::Widget* sender, int count)
    {
        const ItemStack& item = mTradeModel->getItem(mSelectedItem);
        std::string sound = MWWorld::Class::get(item.mBase).getDownSoundId(item.mBase);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        if (item.mType == ItemStack::Type_Barter)
        {
            // this was an item borrowed to us by the merchant
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->returnItem(mSelectedItem, count);
            mTradeModel->returnItemBorrowedToUs(mSelectedItem, count);
        }
        else
        {
            // borrow item to the merchant
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->borrowItem(mSelectedItem, count);
            mTradeModel->borrowItemFromUs(mSelectedItem, count);
        }

        mItemView->update();
    }

    void InventoryWindow::updateItemView()
    {
        mItemView->update();
        mPreviewDirty = true;
    }

    void InventoryWindow::open()
    {
        mPtr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        updateEncumbranceBar();

        mItemView->update();

        notifyContentChanged();
        adjustPanes();
    }

    void InventoryWindow::onWindowResize(MyGUI::Window* _sender)
    {
        adjustPanes();
        std::string setting = "inventory";
        switch(mGuiMode) {
            case GM_Container:
                setting += " container";
                break;
            case GM_Companion:
                setting += " companion";
                break;
            case GM_Barter:
                setting += " barter";
                break;
            default:
                break;
        }

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        float x = _sender->getPosition().left / float(viewSize.width);
        float y = _sender->getPosition().top / float(viewSize.height);
        float w = _sender->getSize().width / float(viewSize.width);
        float h = _sender->getSize().height / float(viewSize.height);
        Settings::Manager::setFloat(setting + " x", "Windows", x);
        Settings::Manager::setFloat(setting + " y", "Windows", y);
        Settings::Manager::setFloat(setting + " w", "Windows", w);
        Settings::Manager::setFloat(setting + " h", "Windows", h);

        if (mMainWidget->getSize().width != mLastXSize || mMainWidget->getSize().height != mLastYSize)
        {
            mLastXSize = mMainWidget->getSize().width;
            mLastYSize = mMainWidget->getSize().height;
            mPreviewDirty = true;
        }
    }

    void InventoryWindow::onFilterChanged(MyGUI::Widget* _sender)
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

        mItemView->update();

        static_cast<MyGUI::Button*>(_sender)->setStateSelected(true);
    }

    void InventoryWindow::onPinToggled()
    {
        MWBase::Environment::get().getWindowManager()->setWeaponVisibility(!mPinned);
    }

    void InventoryWindow::onAvatarClicked(MyGUI::Widget* _sender)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            MWWorld::Ptr ptr = mDragAndDrop->mItem.mBase;
            mDragAndDrop->finish();

            if (mDragAndDrop->mSourceModel != mTradeModel)
            {
                // add item to the player's inventory
                MWWorld::ContainerStore& invStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);
                MWWorld::ContainerStoreIterator it = invStore.begin();

                int origCount = ptr.getRefData().getCount();
                ptr.getRefData().setCount(mDragAndDrop->mDraggedCount);
                it = invStore.add(ptr, mPtr);
                ptr.getRefData().setCount(origCount);

                mDragAndDrop->mSourceModel->removeItem(mDragAndDrop->mItem, mDragAndDrop->mDraggedCount);
                ptr = *it;
            }

            boost::shared_ptr<MWWorld::Action> action = MWWorld::Class::get(ptr).use(ptr);

            action->execute (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

            // this is necessary for books/scrolls: if they are already in the player's inventory,
            // the "Take" button should not be visible.
            // NOTE: the take button is "reset" when the window opens, so we can safely do the following
            // without screwing up future book windows
            MWBase::Environment::get().getWindowManager()->getBookWindow()->setTakeButtonShow(false);
            MWBase::Environment::get().getWindowManager()->getScrollWindow()->setTakeButtonShow(false);

            mItemView->update();

            notifyContentChanged();
        }
        else
        {
            MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance ().getLastPressedPosition (MyGUI::MouseButton::Left);
            MyGUI::IntPoint relPos = mousePos - mAvatar->getAbsolutePosition ();
            int realX = int(float(relPos.left) / float(mAvatar->getSize().width) * 512.f );
            int realY = int(float(relPos.top) / float(mAvatar->getSize().height) * 1024.f );

            MWWorld::Ptr itemSelected = getAvatarSelectedItem (realX, realY);
            if (itemSelected.isEmpty ())
                return;

            for (size_t i=0; i < mTradeModel->getItemCount (); ++i)
            {
                if (mTradeModel->getItem(i).mBase == itemSelected)
                {
                    onItemSelectedFromSourceModel(i);
                    return;
                }
            }
            throw std::runtime_error("Can't find clicked item");
        }
    }

    MWWorld::Ptr InventoryWindow::getAvatarSelectedItem(int x, int y)
    {
        int slot = mPreview.getSlotSelected (x, y);

        if (slot == -1)
            return MWWorld::Ptr();

        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
        if(invStore.getSlot(slot) != invStore.end())
        {
            MWWorld::Ptr item = *invStore.getSlot(slot);
            // NOTE: Don't allow users to select WerewolfRobe objects in the inventory. Vanilla
            // likely uses a hack like this since there's no other way to prevent it from being
            // taken.
            if(item.getCellRef().mRefID == "WerewolfRobe")
                return MWWorld::Ptr();
            return item;
        }

        return MWWorld::Ptr();
    }

    void InventoryWindow::updateEncumbranceBar()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        float capacity = MWWorld::Class::get(player).getCapacity(player);
        float encumbrance = MWWorld::Class::get(player).getEncumbrance(player);
        mEncumbranceBar->setValue(encumbrance, capacity);
    }

    void InventoryWindow::onFrame()
    {
        if (!mMainWidget->getVisible())
            return;

        updateEncumbranceBar();
    }

    int InventoryWindow::getPlayerGold()
    {
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);

        for (MWWorld::ContainerStoreIterator it = invStore.begin();
                it != invStore.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(it->getCellRef().mRefID, "gold_001"))
                return it->getRefData().getCount();
        }
        return 0;
    }

    void InventoryWindow::setTrading(bool trading)
    {
        mTrading = trading;
    }

    void InventoryWindow::doRenderUpdate ()
    {
        if (mPreviewDirty)
        {
            mPreviewDirty = false;
            MyGUI::IntSize size = mAvatar->getSize();

            mPreview.update (size.width, size.height);
            mAvatarImage->setSize(MyGUI::IntSize(std::max(mAvatar->getSize().width, 512), std::max(mAvatar->getSize().height, 1024)));
            mAvatarImage->setImageTexture("CharacterPreview");
        }
    }

    void InventoryWindow::notifyContentChanged()
    {
        // update the spell window just in case new enchanted items were added to inventory
        if (MWBase::Environment::get().getWindowManager()->getSpellWindow())
            MWBase::Environment::get().getWindowManager()->getSpellWindow()->updateSpells();

        mPreviewDirty = true;

        mArmorRating->setCaptionWithReplacing ("#{sArmor}: "
            + boost::lexical_cast<std::string>(static_cast<int>(MWWorld::Class::get(mPtr).getArmorRating(mPtr))));
    }

    void InventoryWindow::pickUpObject (MWWorld::Ptr object)
    {
        // make sure the object is of a type that can be picked up
        std::string type = object.getTypeName();
        if ( (type != typeid(ESM::Apparatus).name())
            && (type != typeid(ESM::Armor).name())
            && (type != typeid(ESM::Book).name())
            && (type != typeid(ESM::Clothing).name())
            && (type != typeid(ESM::Ingredient).name())
            && (type != typeid(ESM::Light).name())
            && (type != typeid(ESM::Miscellaneous).name())
            && (type != typeid(ESM::Lockpick).name())
            && (type != typeid(ESM::Probe).name())
            && (type != typeid(ESM::Repair).name())
            && (type != typeid(ESM::Weapon).name())
            && (type != typeid(ESM::Potion).name()))
            return;

        if (MWWorld::Class::get(object).getName(object) == "") // objects without name presented to user can never be picked up
            return;

        int count = object.getRefData().getCount();
        if (object.getCellRef().mGoldValue > 1)
            count = object.getCellRef().mGoldValue;

        // add to player inventory
        // can't use ActionTake here because we need an MWWorld::Ptr to the newly inserted object
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::Ptr newObject = *MWWorld::Class::get (player).getContainerStore (player).add (object, player);
        // remove from world
        MWBase::Environment::get().getWorld()->deleteObject (object);

        // get ModelIndex to the item
        mTradeModel->update();
        size_t i=0;
        for (; i<mTradeModel->getItemCount(); ++i)
        {
            if (mTradeModel->getItem(i).mBase == newObject)
                break;
        }
        if (i == mTradeModel->getItemCount())
            throw std::runtime_error("Added item not found");
        mDragAndDrop->startDrag(i, mSortModel, mTradeModel, mItemView, count);
    }

    MyGUI::IntCoord InventoryWindow::getAvatarScreenCoord ()
    {
        return mAvatar->getAbsoluteCoord ();
    }
}
