#include "inventorywindow.hpp"

#include <cmath>
#include <stdexcept>

#include <MyGUI_Window.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <osg/Texture2D>

#include <components/misc/stringops.hpp>

#include <components/myguiplatform/myguitexture.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/actionequip.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tradeitemmodel.hpp"
#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "draganddrop.hpp"
#include "widgets.hpp"
#include "tooltips.hpp"

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

    InventoryWindow::InventoryWindow(DragAndDrop* dragAndDrop, osg::Group* parent, Resource::ResourceSystem* resourceSystem)
        : WindowPinnableBase("openmw_inventory_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSelectedItem(-1)
        , mSortModel(nullptr)
        , mTradeModel(nullptr)
        , mGuiMode(GM_Inventory)
        , mLastXSize(0)
        , mLastYSize(0)
        , mPreview(new MWRender::InventoryPreview(parent, resourceSystem, MWMechanics::getPlayer()))
        , mTrading(false)
        , mScaleFactor(1.0f)
        , mUpdateTimer(0.f)
    {
        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        if (uiScale > 1.0)
            mScaleFactor = uiScale;

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
        getWidget(mFilterEdit, "FilterEdit");

        mAvatarImage->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onAvatarClicked);
        mAvatarImage->setRenderItemTexture(mPreviewTexture.get());
        mAvatarImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));

        getWidget(mItemView, "ItemView");
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &InventoryWindow::onItemSelected);
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &InventoryWindow::onBackgroundSelected);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterEdit->eventEditTextChange += MyGUI::newDelegate(this, &InventoryWindow::onNameFilterChanged);

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

        mSortModel->setNameFilter(mFilterEdit->getCaption());

        mItemView->setModel(mSortModel);

        mFilterAll->setStateSelected(true);
        mFilterWeapon->setStateSelected(false);
        mFilterApparel->setStateSelected(false);
        mFilterMagic->setStateSelected(false);
        mFilterMisc->setStateSelected(false);

        mPreview->updatePtr(mPtr);
        mPreview->rebuild();
        mPreview->update();

        dirtyPreview();

        updatePreviewSize();

        updateEncumbranceBar();
        mItemView->update();
        notifyContentChanged();
    }

    void InventoryWindow::clear()
    {
        mPtr = MWWorld::Ptr();
        mTradeModel = nullptr;
        mSortModel = nullptr;
        mItemView->setModel(nullptr);
    }

    void InventoryWindow::toggleMaximized()
    {
        std::string setting = getModeSetting();

        bool maximized = !Settings::Manager::getBool(setting + " maximized", "Windows");
        if (maximized)
            setting += " maximized";

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        float x = Settings::Manager::getFloat(setting + " x", "Windows") * float(viewSize.width);
        float y = Settings::Manager::getFloat(setting + " y", "Windows") * float(viewSize.height);
        float w = Settings::Manager::getFloat(setting + " w", "Windows") * float(viewSize.width);
        float h = Settings::Manager::getFloat(setting + " h", "Windows") * float(viewSize.height);
        MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();
        window->setCoord(x, y, w, h);

        if (maximized)
            Settings::Manager::setBool(setting, "Windows", maximized);
        else
            Settings::Manager::setBool(setting + " maximized", "Windows", maximized);

        adjustPanes();
        updatePreviewSize();
    }

    void InventoryWindow::setGuiMode(GuiMode mode)
    {
        mGuiMode = mode;
        std::string setting = getModeSetting();
        setPinButtonVisible(mode == GM_Inventory);

        if (Settings::Manager::getBool(setting + " maximized", "Windows"))
            setting += " maximized";

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos(static_cast<int>(Settings::Manager::getFloat(setting + " x", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(setting + " y", "Windows") * viewSize.height));
        MyGUI::IntSize size(static_cast<int>(Settings::Manager::getFloat(setting + " w", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(setting + " h", "Windows") * viewSize.height));

        bool needUpdate = (size.width != mMainWidget->getWidth() || size.height != mMainWidget->getHeight());

        mMainWidget->setPosition(pos);
        mMainWidget->setSize(size);

        adjustPanes();

        if (needUpdate)
            updatePreviewSize();
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
                MWBase::Environment::get().getWindowManager()->playSound(sound);
                MWBase::Environment::get().getWindowManager()->messageBox("#{sBarterDialog9}");
                return;
            }

            // check if merchant accepts item
            int services = MWBase::Environment::get().getWindowManager()->getTradeWindow()->getMerchantServices();
            if (!object.getClass().canSell(object, services))
            {
                MWBase::Environment::get().getWindowManager()->playSound(sound);
                MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sBarterDialog4}");
                return;
            }
        }

        // If we unequip weapon during attack, it can lead to unexpected behaviour
        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(mPtr))
        {
            bool isWeapon = item.mBase.getTypeName() == typeid(ESM::Weapon).name();
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);

            if (isWeapon && invStore.isEquipped(item.mBase))
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sCantEquipWeapWarning}");
                return;
            }
        }

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string message = mTrading ? "#{sQuanityMenuMessage01}" : "#{sTake}";
            std::string name = object.getClass().getName(object) + MWGui::ToolTips::getSoulString(object.getCellRef());
            dialog->openCountDialog(name, message, count);
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
                sellItem (nullptr, count);
            else
                dragItem (nullptr, count);
        }
    }

    void InventoryWindow::ensureSelectedItemUnequipped(int count)
    {
        const ItemStack& item = mTradeModel->getItem(mSelectedItem);
        if (item.mType == ItemStack::Type_Equipped)
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
            MWWorld::Ptr newStack = *invStore.unequipItemQuantity(item.mBase, mPtr, count);

            // The unequipped item was re-stacked. We have to update the index
            // since the item pointed does not exist anymore.
            if (item.mBase != newStack)
            {
                updateItemView();  // Unequipping can produce a new stack, not yet in the window...

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
        ensureSelectedItemUnequipped(count);
        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mTradeModel, mItemView, count);
        notifyContentChanged();
    }

    void InventoryWindow::sellItem(MyGUI::Widget* sender, int count)
    {
        ensureSelectedItemUnequipped(count);
        const ItemStack& item = mTradeModel->getItem(mSelectedItem);
        std::string sound = item.mBase.getClass().getUpSoundId(item.mBase);
        MWBase::Environment::get().getWindowManager()->playSound(sound);

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

    void InventoryWindow::onOpen()
    {
        // Reset the filter focus when opening the window
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (focus == mFilterEdit)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);

        if (!mPtr.isEmpty())
        {
            updateEncumbranceBar();
            mItemView->update();
            notifyContentChanged();
        }
        adjustPanes();
    }

    std::string InventoryWindow::getModeSetting() const
    {
        std::string setting = "inventory";
        switch(mGuiMode)
        {
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

        return setting;
    }

    void InventoryWindow::onWindowResize(MyGUI::Window* _sender)
    {
        adjustPanes();
        std::string setting = getModeSetting();

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        float x = _sender->getPosition().left / float(viewSize.width);
        float y = _sender->getPosition().top / float(viewSize.height);
        float w = _sender->getSize().width / float(viewSize.width);
        float h = _sender->getSize().height / float(viewSize.height);
        Settings::Manager::setFloat(setting + " x", "Windows", x);
        Settings::Manager::setFloat(setting + " y", "Windows", y);
        Settings::Manager::setFloat(setting + " w", "Windows", w);
        Settings::Manager::setFloat(setting + " h", "Windows", h);
        bool maximized = Settings::Manager::getBool(setting + " maximized", "Windows");
        if (maximized)
            Settings::Manager::setBool(setting + " maximized", "Windows", false);

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
        mPreview->setViewport(int(width*mScaleFactor), int(height*mScaleFactor));

        mAvatarImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f,
                                                                     width*mScaleFactor/float(mPreview->getTextureWidth()), height*mScaleFactor/float(mPreview->getTextureHeight())));
    }

    void InventoryWindow::onNameFilterChanged(MyGUI::EditBox* _sender)
    {
        mSortModel->setNameFilter(_sender->getCaption());
        mItemView->update();
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
        Settings::Manager::setBool("inventory pin", "Windows", mPinned);

        MWBase::Environment::get().getWindowManager()->setWeaponVisibility(!mPinned);
    }

    void InventoryWindow::onTitleDoubleClicked()
    {
        if (MyGUI::InputManager::getInstance().isShiftPressed())
            toggleMaximized();
        else if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Inventory);
    }

    void InventoryWindow::useItem(const MWWorld::Ptr &ptr, bool force)
    {
        const std::string& script = ptr.getClass().getScript(ptr);
        if (!script.empty())
        {
            // Don't try to equip the item if PCSkipEquip is set to 1
            if (ptr.getRefData().getLocals().getIntVar(script, "pcskipequip") == 1)
            {
                ptr.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);
                return;
            }
            ptr.getRefData().getLocals().setVarByInt(script, "onpcequip", 0);
        }

        MWWorld::Ptr player = MWMechanics::getPlayer();

        // early-out for items that need to be equipped, but can't be equipped: we don't want to set OnPcEquip in that case
        if (!ptr.getClass().getEquipmentSlots(ptr).first.empty())
        {
            if (ptr.getClass().hasItemHealth(ptr) && ptr.getCellRef().getCharge() == 0)
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sInventoryMessage1}");
                updateItemView();
                return;
            }

            if (!force)
            {
                std::pair<int, std::string> canEquip = ptr.getClass().canBeEquipped(ptr, player);

                if (canEquip.first == 0)
                {
                    MWBase::Environment::get().getWindowManager()->messageBox(canEquip.second);
                    updateItemView();
                    return;
                }
            }
        }

        // If the item has a script, set OnPCEquip or PCSkipEquip to 1
        if (!script.empty())
        {
            // Ingredients, books and repair hammers must not have OnPCEquip set to 1 here
            const std::string& type = ptr.getTypeName();
            bool isBook = type == typeid(ESM::Book).name();
            if (!isBook && type != typeid(ESM::Ingredient).name() && type != typeid(ESM::Repair).name())
                ptr.getRefData().getLocals().setVarByInt(script, "onpcequip", 1);
            // Books must have PCSkipEquip set to 1 instead
            else if (isBook)
                ptr.getRefData().getLocals().setVarByInt(script, "pcskipequip", 1);
        }

        std::shared_ptr<MWWorld::Action> action = ptr.getClass().use(ptr, force);
        action->execute(player);

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

            // If item is ingredient or potion don't stop drag and drop to simplify action of taking more than one 1 item
            if ((ptr.getTypeName() == typeid(ESM::Potion).name() ||
                 ptr.getTypeName() == typeid(ESM::Ingredient).name())
                && mDragAndDrop->mDraggedCount > 1)
            {
                // Item can be provided from other window for example container.
                // But after DragAndDrop::startDrag item automaticly always gets to player inventory.
                mSelectedItem = getModel()->getIndex(mDragAndDrop->mItem);
                dragItem(nullptr, mDragAndDrop->mDraggedCount - 1);
            }
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

        // Scale coordinates
        x = int(x*mScaleFactor);
        y = int(y*mScaleFactor);

        int slot = mPreview->getSlotSelected (x, y);

        if (slot == -1)
            return MWWorld::Ptr();

        MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
        if(invStore.getSlot(slot) != invStore.end())
        {
            MWWorld::Ptr item = *invStore.getSlot(slot);
            if (!item.getClass().showsInInventory(item))
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
        mEncumbranceBar->setValue(std::ceil(encumbrance), static_cast<int>(capacity));
    }

    void InventoryWindow::onFrame(float dt)
    {
        updateEncumbranceBar();

        if (mPinned)
        {
            mUpdateTimer += dt;
            if (0.1f < mUpdateTimer)
            {
                mUpdateTimer = 0;

                // Update pinned inventory in-game
                if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
                {
                    mItemView->update();
                    notifyContentChanged();
                }
            }
        }
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

        // An object that can be picked up must have a tooltip.
        if (!object.getClass().hasToolTip(object))
            return;

        int count = object.getRefData().getCount();
        if (object.getClass().isGold(object))
            count *= object.getClass().getValue(object);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWBase::Environment::get().getWorld()->breakInvisibility(player);
        
        if (!object.getRefData().activate())
            return;

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
        MWWorld::Ptr player = MWMechanics::getPlayer();

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player))
            return;

        const MWMechanics::CreatureStats &stats = player.getClass().getCreatureStats(player);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead() || stats.getHitRecovery())
            return;

        ItemModel::ModelIndex selected = -1;
        // not using mSortFilterModel as we only need sorting, not filtering
        SortFilterItemModel model(new InventoryItemModel(player));
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

            if (item.getClass().getTypeName() == typeid(ESM::Weapon).name() &&
                isRightHandWeapon(item) &&
                item.getClass().canBeEquipped(item, player).first)
            {
                found = true;
                break;
            }
        }

        if (!found || selected == cycled)
            return;

        useItem(model.getItem(cycled).mBase);
    }

    void InventoryWindow::rebuildAvatar()
    {
        mPreview->rebuild();
    }
}
