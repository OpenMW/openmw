#include "inventorywindow.hpp"

#include <stdexcept>

#include <MyGUI_Window.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Button.h>

#include <osg/Texture2D>

#include <components/misc/stringops.hpp>

#include <components/myguiplatform/myguitexture.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/action.hpp"
#include "../mwscript/interpretercontext.hpp"
#include "../mwrender/characterpreview.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "draganddrop.hpp"
#include "widgets.hpp"

namespace
{

    bool isRightHandWeapon(const MWWorld::Ptr& item)
    {
        if (item.getClass().getTypeName() != typeid(ESM::Weapon).name())
            return false;
        std::vector<int> equipmentSlots = item.getClass().getEquipmentSlots(item).first;
        return (!equipmentSlots.empty() && equipmentSlots.front() == MWWorld::InventoryStore::Slot_CarriedRight);
    }

}

namespace MWGui
{

    InventoryWindow::InventoryWindow(DragAndDrop* dragAndDrop, osgViewer::Viewer* viewer, Resource::ResourceSystem* resourceSystem)
        : WindowPinnableBase("openmw_inventory_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSelectedItem(-1)
        , mSortModel(NULL)
        , mTradeModel(NULL)
        , mGuiMode(GM_Inventory)
        , mLastXSize(0)
        , mLastYSize(0)
        , mPreview(new MWRender::InventoryPreview(viewer, resourceSystem, MWMechanics::getPlayer()))
        , mTrading(false)
    {
        mPreviewTexture.reset(new osgMyGUI::OSGTexture(mPreview->getTexture()));
        mPreview->rebuild();

        mMainWidget->castType<MyGUI::Window>()->eventWindowChangeCoord += MyGUI::newDelegate(this, &InventoryWindow::onWindowResize);

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
        mAvatarImage->setRenderItemTexture(mPreviewTexture.get());
        mAvatarImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 1.f, 1.f, 0.f));

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
        int leftPaneWidth = static_cast<int>((mMainWidget->getSize().height - 44 - mArmorRating->getHeight()) * aspect);
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

        if (mSortModel) // reuse existing SortModel when possible to keep previous category/filter settings
            mSortModel->setSourceModel(mTradeModel);
        else
            mSortModel = new SortFilterItemModel(mTradeModel);

        mItemView->setModel(mSortModel);

        mPreview->updatePtr(mPtr);
        mPreview->rebuild();
        mPreview->update();

        dirtyPreview();

        updatePreviewSize();
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
        MyGUI::IntPoint pos(static_cast<int>(Settings::Manager::getFloat(setting + " x", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(setting + " y", "Windows") * viewSize.height));
        MyGUI::IntSize size(static_cast<int>(Settings::Manager::getFloat(setting + " w", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(setting + " h", "Windows") * viewSize.height));

        mMainWidget->setPosition(pos);
        mMainWidget->setSize(size);

        if (size.width != mMainWidget->getWidth() || size.height != mMainWidget->getHeight())
            updatePreviewSize();

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

        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        if (mTrading)
        {
            // Can't give conjured items to a merchant
            if (item.mFlags & ItemStack::Flag_Bound)
            {
                MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                MWBase::Environment::get().getWindowManager()->messageBox("#{sBarterDialog9}");
                return;
            }

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
            dialog->openCountDialog(object.getClass().getName(object), message, count);
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
        notifyContentChanged();
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
            mTradeModel->returnItemBorrowedToUs(mSelectedItem, count);
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->returnItem(mSelectedItem, count);
        }
        else
        {
            // borrow item to the merchant
            mTradeModel->borrowItemFromUs(mSelectedItem, count);
            MWBase::Environment::get().getWindowManager()->getTradeWindow()->borrowItem(mSelectedItem, count);
        }

        mItemView->update();
        notifyContentChanged();
    }

    void InventoryWindow::updateItemView()
    {
        MWBase::Environment::get().getWindowManager()->updateSpellWindow();

        mItemView->update();

        dirtyPreview();
    }

    void InventoryWindow::open()
    {
        mPtr = MWMechanics::getPlayer();

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

            updatePreviewSize();
            updateArmorRating();
        }
    }

    void InventoryWindow::updateArmorRating()
    {
        mArmorRating->setCaptionWithReplacing ("#{sArmor}: "
            + MyGUI::utility::toString(static_cast<int>(mPtr.getClass().getArmorRating(mPtr))));
        if (mArmorRating->getTextSize().width > mArmorRating->getSize().width)
            mArmorRating->setCaptionWithReplacing (MyGUI::utility::toString(static_cast<int>(mPtr.getClass().getArmorRating(mPtr))));
    }

    void InventoryWindow::updatePreviewSize()
    {
        MyGUI::IntSize size = mAvatarImage->getSize();
        int width = std::min(mPreview->getTextureWidth(), size.width);
        int height = std::min(mPreview->getTextureHeight(), size.height);
        mPreview->setViewport(width, height);

        mAvatarImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, height/float(mPreview->getTextureHeight()),
                                                                     width/float(mPreview->getTextureWidth()), 0.f));
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

        _sender->castType<MyGUI::Button>()->setStateSelected(true);
    }

    void InventoryWindow::onPinToggled()
    {
        MWBase::Environment::get().getWindowManager()->setWeaponVisibility(!mPinned);
    }

    void InventoryWindow::onTitleDoubleClicked()
    {
        if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Inventory);
    }

    void InventoryWindow::useItem(const MWWorld::Ptr &ptr)
    {
        const std::string& script = ptr.getClass().getScript(ptr);

        MWWorld::Ptr player = MWMechanics::getPlayer();

        // early-out for items that need to be equipped, but can't be equipped: we don't want to set OnPcEquip in that case
        if (!ptr.getClass().getEquipmentSlots(ptr).first.empty())
        {
            std::pair<int, std::string> canEquip = ptr.getClass().canBeEquipped(ptr, player);
            if (canEquip.first == 0)
            {
                MWBase::Environment::get().getWindowManager()->messageBox(canEquip.second);
                updateItemView();
                return;
            }
        }

        // If the item has a script, set its OnPcEquip to 1
        if (!script.empty()
                // Another morrowind oddity: when an item has skipped equipping and pcskipequip is reset to 0 afterwards,
                // the next time it is equipped will work normally, but will not set onpcequip
                && (ptr != mSkippedToEquip || ptr.getRefData().getLocals().getIntVar(script, "pcskipequip") == 1))
            ptr.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);

        // Give the script a chance to run once before we do anything else
        // this is important when setting pcskipequip as a reaction to onpcequip being set (bk_treasuryreport does this)
        if (!script.empty() && MWBase::Environment::get().getWorld()->getScriptsEnabled())
        {
            MWScript::InterpreterContext interpreterContext (&ptr.getRefData().getLocals(), ptr);
            MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
        }

        if (script.empty() || ptr.getRefData().getLocals().getIntVar(script, "pcskipequip") == 0)
        {
            boost::shared_ptr<MWWorld::Action> action = ptr.getClass().use(ptr);

            action->execute (player);

            mSkippedToEquip = MWWorld::Ptr();
        }
        else
            mSkippedToEquip = ptr;

        if (isVisible())
        {
            mItemView->update();

            notifyContentChanged();
        }
        // else: will be updated in open()
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

            MWWorld::Ptr itemSelected = getAvatarSelectedItem (relPos.left, relPos.top);
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
        // convert to OpenGL lower-left origin
        y = (mAvatarImage->getHeight()-1) - y;
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
        MWWorld::Ptr player = MWMechanics::getPlayer();

        float capacity = player.getClass().getCapacity(player);
        float encumbrance = player.getClass().getEncumbrance(player);
        mTradeModel->adjustEncumbrance(encumbrance);
        mEncumbranceBar->setValue(static_cast<int>(encumbrance), static_cast<int>(capacity));
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

    void InventoryWindow::dirtyPreview()
    {
        mPreview->update();

        updateArmorRating();
    }

    void InventoryWindow::notifyContentChanged()
    {
        // update the spell window just in case new enchanted items were added to inventory
        MWBase::Environment::get().getWindowManager()->updateSpellWindow();

        MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(
                    MWMechanics::getPlayer());

        dirtyPreview();
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

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWBase::Environment::get().getWorld()->breakInvisibility(player);

        MWBase::Environment::get().getMechanicsManager()->itemTaken(player, object, MWWorld::Ptr(), count);

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

        MWBase::Environment::get().getWindowManager()->updateSpellWindow();
    }

    void InventoryWindow::cycle(bool next)
    {
        ItemModel::ModelIndex selected = -1;
        // not using mSortFilterModel as we only need sorting, not filtering
        SortFilterItemModel model(new InventoryItemModel(MWMechanics::getPlayer()));
        model.setSortByType(false);
        model.update();
        if (model.getItemCount() == 0)
            return;

        for (ItemModel::ModelIndex i=0; i<int(model.getItemCount()); ++i)
        {
            MWWorld::Ptr item = model.getItem(i).mBase;
            if (model.getItem(i).mType & ItemStack::Type_Equipped && isRightHandWeapon(item))
                selected = i;
        }

        int incr = next ? 1 : -1;
        bool found = false;
        std::string lastId;
        if (selected != -1)
            lastId = model.getItem(selected).mBase.getCellRef().getRefId();
        ItemModel::ModelIndex cycled = selected;
        for (unsigned int i=0; i<model.getItemCount(); ++i)
        {
            cycled += incr;
            cycled = (cycled + model.getItemCount()) % model.getItemCount();

            MWWorld::Ptr item = model.getItem(cycled).mBase;

            // skip different stacks of the same item, or we will get stuck as stacking/unstacking them may change their relative ordering
            if (Misc::StringUtils::ciEqual(lastId, item.getCellRef().getRefId()))
                continue;

            lastId = item.getCellRef().getRefId();

            if (item.getClass().getTypeName() == typeid(ESM::Weapon).name() && isRightHandWeapon(item))
            {
                found = true;
                break;
            }
        }

        if (!found)
            return;

        useItem(model.getItem(cycled).mBase);
    }

    void InventoryWindow::rebuildAvatar()
    {
        mPreview->rebuild();
    }
}
