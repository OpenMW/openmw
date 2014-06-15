#include "inventorywindow.hpp"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"
#include "../mwscript/interpretercontext.hpp"
#include "../mwbase/scriptmanager.hpp"

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
        , mPreview(new MWRender::InventoryPreview(MWBase::Environment::get().getWorld ()->getPlayerPtr()))
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

        mAvatarImage->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onAvatarClicked);

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
        const float aspect = 0.5; // fixed aspect ratio for the avatar image
        float leftPaneWidth = (mMainWidget->getSize().height-44-mArmorRating->getHeight()) * aspect;
        mLeftPane->setSize( leftPaneWidth, mMainWidget->getSize().height-44 );
        mRightPane->setCoord( mLeftPane->getPosition().left + leftPaneWidth + 4,
                              mRightPane->getPosition().top,
                              mMainWidget->getSize().width - 12 - leftPaneWidth - 15,
                              mMainWidget->getSize().height-44 );
    }

    void InventoryWindow::updatePlayer()
    {
        mPtr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        mTradeModel = new TradeItemModel(new InventoryItemModel(mPtr), MWWorld::Ptr());
        mSortModel = new SortFilterItemModel(mTradeModel);
        mItemView->setModel(mSortModel);
        mPreview.reset(new MWRender::InventoryPreview(mPtr));
        mPreview->setup();
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

    SortFilterItemModel* InventoryWindow::getSortFilterModel()
    {
        return mSortModel;
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
        std::string sound = item.mBase.getClass().getDownSoundId(item.mBase);

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;

        // Bound items may not be moved
        if (item.mBase.getCellRef().getRefId().size() > 6
                && item.mBase.getCellRef().getRefId().substr(0,6) == "bound_")
        {
            MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
            MWBase::Environment::get().getWindowManager()->messageBox("#{sBarterDialog12}");
            return;
        }

        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        if (mTrading)
        {
            // check if merchant accepts item
            int services = MWBase::Environment::get().getWindowManager()->getTradeWindow()->getMerchantServices();
            if (!object.getClass().canSell(object, services))
            {
                MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sBarterDialog4}");
                return;
            }
        }

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string message = mTrading ? "#{sQuanityMenuMessage01}" : "#{sTake}";
            dialog->open(object.getClass().getName(object), message, count);
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

    void InventoryWindow::ensureSelectedItemUnequipped()
    {
        const ItemStack& item = mTradeModel->getItem(mSelectedItem);
        if (item.mType == ItemStack::Type_Equipped)
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
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

                mSelectedItem = newIndex;
            }
        }
    }

    void InventoryWindow::dragItem(MyGUI::Widget* sender, int count)
    {
        ensureSelectedItemUnequipped();
        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mTradeModel, mItemView, count);
    }

    void InventoryWindow::sellItem(MyGUI::Widget* sender, int count)
    {
        ensureSelectedItemUnequipped();
        const ItemStack& item = mTradeModel->getItem(mSelectedItem);
        std::string sound = item.mBase.getClass().getDownSoundId(item.mBase);
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
        mPtr = MWBase::Environment::get().getWorld()->getPlayerPtr();

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

    void InventoryWindow::useItem(const MWWorld::Ptr &ptr)
    {
        const std::string& script = ptr.getClass().getScript(ptr);

        // If the item has a script, set its OnPcEquip to 1
        if (!script.empty()
                // Another morrowind oddity: when an item has skipped equipping and pcskipequip is reset to 0 afterwards,
                // the next time it is equipped will work normally, but will not set onpcequip
                && (ptr != mSkippedToEquip || ptr.getRefData().getLocals().getIntVar(script, "pcskipequip") == 1))
            ptr.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);

        // Give the script a chance to run once before we do anything else
        // this is important when setting pcskipequip as a reaction to onpcequip being set (bk_treasuryreport does this)
        if (!script.empty())
        {
            MWScript::InterpreterContext interpreterContext (&ptr.getRefData().getLocals(), ptr);
            MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
        }

        if (script.empty() || ptr.getRefData().getLocals().getIntVar(script, "pcskipequip") == 0)
        {
            boost::shared_ptr<MWWorld::Action> action = ptr.getClass().use(ptr);

            action->execute (MWBase::Environment::get().getWorld()->getPlayerPtr());

            // this is necessary for books/scrolls: if they are already in the player's inventory,
            // the "Take" button should not be visible.
            // NOTE: the take button is "reset" when the window opens, so we can safely do the following
            // without screwing up future book windows
            MWBase::Environment::get().getWindowManager()->getBookWindow()->setTakeButtonShow(false);
            MWBase::Environment::get().getWindowManager()->getScrollWindow()->setTakeButtonShow(false);

            mSkippedToEquip = MWWorld::Ptr();
        }
        else
            mSkippedToEquip = ptr;

        mItemView->update();

        notifyContentChanged();
    }

    void InventoryWindow::onAvatarClicked(MyGUI::Widget* _sender)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            MWWorld::Ptr ptr = mDragAndDrop->mItem.mBase;
            mDragAndDrop->finish();

            if (mDragAndDrop->mSourceModel != mTradeModel)
            {
                // Move item to the player's inventory
                ptr = mDragAndDrop->mSourceModel->moveItem(mDragAndDrop->mItem, mDragAndDrop->mDraggedCount, mTradeModel);
            }
            useItem(ptr);
        }
        else
        {
            MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance ().getLastPressedPosition (MyGUI::MouseButton::Left);
            MyGUI::IntPoint relPos = mousePos - mAvatarImage->getAbsolutePosition ();
            int realX = int(float(relPos.left) / float(mAvatarImage->getSize().width) * 512.f );
            int realY = int(float(relPos.top) / float(mAvatarImage->getSize().height) * 1024.f );

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
        int slot = mPreview->getSlotSelected (x, y);

        if (slot == -1)
            return MWWorld::Ptr();

        MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
        if(invStore.getSlot(slot) != invStore.end())
        {
            MWWorld::Ptr item = *invStore.getSlot(slot);
            // NOTE: Don't allow users to select WerewolfRobe objects in the inventory. Vanilla
            // likely uses a hack like this since there's no other way to prevent it from being
            // taken.
            if(item.getCellRef().getRefId() == "werewolfrobe")
                return MWWorld::Ptr();
            return item;
        }

        return MWWorld::Ptr();
    }

    void InventoryWindow::updateEncumbranceBar()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();

        float capacity = player.getClass().getCapacity(player);
        float encumbrance = player.getClass().getEncumbrance(player);
        mEncumbranceBar->setValue(encumbrance, capacity);
    }

    void InventoryWindow::onFrame()
    {
        if (!mMainWidget->getVisible())
            return;

        updateEncumbranceBar();
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
            MyGUI::IntSize size = mAvatarImage->getSize();

            mPreview->update (size.width, size.height);

            mAvatarImage->setImageTexture("CharacterPreview");
            mAvatarImage->setImageCoord(MyGUI::IntCoord(0, 0, std::min(512, size.width), std::min(1024, size.height)));
            mAvatarImage->setImageTile(MyGUI::IntSize(std::min(512, size.width), std::min(1024, size.height)));

            mArmorRating->setCaptionWithReplacing ("#{sArmor}: "
                + boost::lexical_cast<std::string>(static_cast<int>(mPtr.getClass().getArmorRating(mPtr))));
            if (mArmorRating->getTextSize().width > mArmorRating->getSize().width)
                mArmorRating->setCaptionWithReplacing (boost::lexical_cast<std::string>(static_cast<int>(mPtr.getClass().getArmorRating(mPtr))));
        }
    }

    void InventoryWindow::notifyContentChanged()
    {
        // update the spell window just in case new enchanted items were added to inventory
        if (MWBase::Environment::get().getWindowManager()->getSpellWindow())
            MWBase::Environment::get().getWindowManager()->getSpellWindow()->updateSpells();

        mPreviewDirty = true;
    }

    void InventoryWindow::pickUpObject (MWWorld::Ptr object)
    {
        // If the inventory is not yet enabled, don't pick anything up
        if (!MWBase::Environment::get().getWindowManager()->isAllowed(GW_Inventory))
            return;
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

        if (object.getClass().getName(object) == "") // objects without name presented to user can never be picked up
            return;

        int count = object.getRefData().getCount();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWBase::Environment::get().getWorld()->breakInvisibility(player);

        // add to player inventory
        // can't use ActionTake here because we need an MWWorld::Ptr to the newly inserted object
        MWWorld::Ptr newObject = *player.getClass().getContainerStore (player).add (object, object.getRefData().getCount(), player);
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

        MWBase::Environment::get().getMechanicsManager()->itemTaken(player, newObject, count);
    }
}
