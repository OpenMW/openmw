#include "inventorywindow.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <cassert>

#include <boost/lexical_cast.hpp>

#include <components/compiler/locals.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/inventorystore.hpp"

#include "widgets.hpp"
#include "bookwindow.hpp"
#include "scrollwindow.hpp"
#include "spellwindow.hpp"

namespace MWGui
{

    InventoryWindow::InventoryWindow(MWBase::WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
        : ContainerBase(dragAndDrop)
        , WindowPinnableBase("openmw_inventory_window.layout", parWindowManager)
        , mTrading(false)
        , mLastXSize(0)
        , mLastYSize(0)
        , mPreview(MWBase::Environment::get().getWorld ()->getPlayer ().getPlayer ())
    {
        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &InventoryWindow::onWindowResize);

        getWidget(mAvatar, "Avatar");
        getWidget(mAvatarImage, "AvatarImage");
        getWidget(mEncumbranceBar, "EncumbranceBar");
        getWidget(mEncumbranceText, "EncumbranceBarT");
        getWidget(mFilterAll, "AllButton");
        getWidget(mFilterWeapon, "WeaponButton");
        getWidget(mFilterApparel, "ApparelButton");
        getWidget(mFilterMagic, "MagicButton");
        getWidget(mFilterMisc, "MiscButton");
        getWidget(mLeftPane, "LeftPane");
        getWidget(mRightPane, "RightPane");
        getWidget(mArmorRating, "ArmorRating");

        mAvatar->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onAvatarClicked);

        MyGUI::ScrollView* itemView;
        MyGUI::Widget* containerWidget;
        getWidget(containerWidget, "Items");
        getWidget(itemView, "ItemView");
        setWidgets(containerWidget, itemView);

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);

        mFilterAll->setStateSelected(true);

        setCoord(0, 342, 498, 258);

        mPreview.setup();

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        openContainer(player);
    }

    void InventoryWindow::open()
    {
        updateEncumbranceBar();

        mTrading = false;

        mBoughtItems.clear();

        onWindowResize(static_cast<MyGUI::Window*>(mMainWidget));
        drawItems();
    }

    void InventoryWindow::onWindowResize(MyGUI::Window* _sender)
    {
        const float aspect = 0.5; // fixed aspect ratio for the left pane
        mLeftPane->setSize( (_sender->getSize().height-44) * aspect, _sender->getSize().height-44 );
        mRightPane->setCoord( mLeftPane->getPosition().left + (_sender->getSize().height-44) * aspect + 4,
                              mRightPane->getPosition().top,
                              _sender->getSize().width - 12 - (_sender->getSize().height-44) * aspect - 15,
                              _sender->getSize().height-44 );

        if (mMainWidget->getSize().width != mLastXSize || mMainWidget->getSize().height != mLastYSize)
        {
            drawItems();
            mLastXSize = mMainWidget->getSize().width;
            mLastYSize = mMainWidget->getSize().height;
        }
    }

    void InventoryWindow::onFilterChanged(MyGUI::Widget* _sender)
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

    void InventoryWindow::onPinToggled()
    {
        mWindowManager.setWeaponVisibility(!mPinned);
    }

    void InventoryWindow::onAvatarClicked(MyGUI::Widget* _sender)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            MWWorld::Ptr ptr = *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>();

            if (mDragAndDrop->mDraggedFrom != this)
            {
                // add item to the player's inventory
                MWWorld::ContainerStore& invStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);
                MWWorld::ContainerStoreIterator it = invStore.begin();

                int origCount = ptr.getRefData().getCount();
                ptr.getRefData().setCount(origCount - mDragAndDrop->mDraggedCount);
                it = invStore.add(ptr);
                (*it).getRefData().setCount(mDragAndDrop->mDraggedCount);
                ptr = *it;
            }

            /// \todo scripts

            boost::shared_ptr<MWWorld::Action> action = MWWorld::Class::get(ptr).use(ptr);

            action->execute (MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

            // this is necessary for books/scrolls: if they are already in the player's inventory,
            // the "Take" button should not be visible.
            // NOTE: the take button is "reset" when the window opens, so we can safely do the following
            // without screwing up future book windows
            mWindowManager.getBookWindow()->setTakeButtonShow(false);
            mWindowManager.getScrollWindow()->setTakeButtonShow(false);

            mDragAndDrop->mIsOnDragAndDrop = false;
            MyGUI::Gui::getInstance().destroyWidget(mDragAndDrop->mDraggedWidget);

            mWindowManager.setDragDrop(false);

            drawItems();

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

            for (unsigned int i=0; i < mContainerWidget->getChildCount (); ++i)
            {
                MyGUI::Widget* w = mContainerWidget->getChildAt (i);

                if (*w->getUserData<MWWorld::Ptr>() == itemSelected)
                {
                    onSelectedItem(w);
                    return;
                }
            }
        }
    }

    MWWorld::Ptr InventoryWindow::getAvatarSelectedItem(int x, int y)
    {
        int slot = mPreview.getSlotSelected (x, y);

        if (slot == -1)
            return MWWorld::Ptr();

        MWWorld::Ptr player = mPtr;
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(player).getInventoryStore(player);
        if (invStore.getSlot(slot) != invStore.end())
            return *invStore.getSlot (slot);
        else
            return MWWorld::Ptr();
    }

    void InventoryWindow::_unequipItem(MWWorld::Ptr item)
    {
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);

        for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
            if (it != invStore.end() && *it == item)
            {
                invStore.equip(slot, invStore.end());
                std::string script = MWWorld::Class::get(*it).getScript(*it);
                
                // Unset OnPCEquip Variable on item's script, if it has a script with that variable declared
                if(script != "")
                    (*it).mRefData->getLocals().setVarByInt(script, "onpcequip", 0);
                
                return;
            }
        }
    }

    void InventoryWindow::updateEncumbranceBar()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        float capacity = MWWorld::Class::get(player).getCapacity(player);
        float encumbrance = MWWorld::Class::get(player).getEncumbrance(player);
        mEncumbranceBar->setProgressRange(capacity);
        mEncumbranceBar->setProgressPosition(encumbrance);
        mEncumbranceText->setCaption( boost::lexical_cast<std::string>(int(encumbrance)) + "/" + boost::lexical_cast<std::string>(int(capacity)) );
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

    void InventoryWindow::startTrade()
    {
        mTrading = true;
    }

    void InventoryWindow::notifyContentChanged()
    {
        // update the spell window just in case new enchanted items were added to inventory
        if (mWindowManager.getSpellWindow())
            mWindowManager.getSpellWindow()->updateSpells();

        // update selected weapon icon
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weaponSlot = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if (weaponSlot == invStore.end())
            mWindowManager.unsetSelectedWeapon();
        else
            mWindowManager.setSelectedWeapon(*weaponSlot, 100); /// \todo track weapon durability

        MyGUI::IntSize size = mAvatar->getSize();

        mPreview.update (size.width, size.height);
        mAvatarImage->setSize(MyGUI::IntSize(std::max(mAvatar->getSize().width, 512), std::max(mAvatar->getSize().height, 1024)));
        mAvatarImage->setImageTexture("CharacterPreview");

        mArmorRating->setCaptionWithReplacing ("#{sArmor}: "
            + boost::lexical_cast<std::string>(static_cast<int>(MWWorld::Class::get(mPtr).getArmorRating(mPtr))));
    }

    void InventoryWindow::pickUpObject (MWWorld::Ptr object)
    {
        /// \todo scripts

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

        // sound
        std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound(sound, 1, 1);

        int count = object.getRefData().getCount();

        // add to player inventory
        // can't use ActionTake here because we need an MWWorld::Ptr to the newly inserted object
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::Ptr newObject = *MWWorld::Class::get (player).getContainerStore (player).add (object);
        // remove from world
        MWBase::Environment::get().getWorld()->deleteObject (object);

        mDragAndDrop->mIsOnDragAndDrop = true;
        mDragAndDrop->mDraggedCount = count;

        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(newObject).getInventoryIcon(newObject);
        MyGUI::ImageBox* baseWidget = mContainerWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 42, 42), MyGUI::Align::Default);
        baseWidget->detachFromWidget();
        baseWidget->attachToWidget(mDragAndDrop->mDragAndDropWidget);
        baseWidget->setUserData(newObject);
        mDragAndDrop->mDraggedWidget = baseWidget;
        MyGUI::ImageBox* image = baseWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        image->setImageTexture(path);
        image->setNeedMouseFocus(false);

        // text widget that shows item count
        MyGUI::TextBox* text = image->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
        text->setTextAlign(MyGUI::Align::Right);
        text->setNeedMouseFocus(false);
        text->setTextShadow(true);
        text->setTextShadowColour(MyGUI::Colour(0,0,0));
        text->setCaption(getCountString(count));
        mDragAndDrop->mDraggedFrom = this;
    }

    MyGUI::IntCoord InventoryWindow::getAvatarScreenCoord ()
    {
        return mAvatar->getAbsoluteCoord ();
    }
}
