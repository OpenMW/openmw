#include "inventorywindow.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <cassert>

#include <boost/lexical_cast.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwsound/soundmanager.hpp"

#include "../mwclass/container.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"
#include "bookwindow.hpp"
#include "scrollwindow.hpp"
#include "spellwindow.hpp"

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }
}

namespace MWGui
{

    InventoryWindow::InventoryWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
        : ContainerBase(dragAndDrop)
        , WindowPinnableBase("openmw_inventory_window.layout", parWindowManager)
        , mTrading(false)
    {
        static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &InventoryWindow::onWindowResize);

        getWidget(mAvatar, "Avatar");
        getWidget(mEncumbranceBar, "EncumbranceBar");
        getWidget(mEncumbranceText, "EncumbranceBarT");
        getWidget(mFilterAll, "AllButton");
        getWidget(mFilterWeapon, "WeaponButton");
        getWidget(mFilterApparel, "ApparelButton");
        getWidget(mFilterMagic, "MagicButton");
        getWidget(mFilterMisc, "MiscButton");
        getWidget(mLeftPane, "LeftPane");
        getWidget(mRightPane, "RightPane");

        mAvatar->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onAvatarClicked);

        MyGUI::ScrollView* itemView;
        MyGUI::Widget* containerWidget;
        getWidget(containerWidget, "Items");
        getWidget(itemView, "ItemView");
        setWidgets(containerWidget, itemView);

        // adjust size of buttons to fit text
        int curX = 0;
        mFilterAll->setSize( mFilterAll->getTextSize().width + 24, mFilterAll->getSize().height );
        curX += mFilterAll->getTextSize().width + 24 + 4;

        mFilterWeapon->setPosition(curX, mFilterWeapon->getPosition().top);
        mFilterWeapon->setSize( mFilterWeapon->getTextSize().width + 24, mFilterWeapon->getSize().height );
        curX += mFilterWeapon->getTextSize().width + 24 + 4;

        mFilterApparel->setPosition(curX, mFilterApparel->getPosition().top);
        mFilterApparel->setSize( mFilterApparel->getTextSize().width + 24, mFilterApparel->getSize().height );
        curX += mFilterApparel->getTextSize().width + 24 + 4;

        mFilterMagic->setPosition(curX, mFilterMagic->getPosition().top);
        mFilterMagic->setSize( mFilterMagic->getTextSize().width + 24, mFilterMagic->getSize().height );
        curX += mFilterMagic->getTextSize().width + 24 + 4;

        mFilterMisc->setPosition(curX, mFilterMisc->getPosition().top);
        mFilterMisc->setSize( mFilterMisc->getTextSize().width + 24, mFilterMisc->getSize().height );

        mFilterAll->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterWeapon->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterApparel->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMagic->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);
        mFilterMisc->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onFilterChanged);

        mFilterAll->setStateSelected(true);

        setCoord(0, 342, 498, 258);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        openContainer(player);
    }

    void InventoryWindow::open()
    {
        updateEncumbranceBar();

        mTrading = false;

        mBoughtItems.clear();

        onWindowResize(static_cast<MyGUI::Window*>(mMainWidget));
    }

    void InventoryWindow::onWindowResize(MyGUI::Window* _sender)
    {
        const float aspect = 0.5; // fixed aspect ratio for the left pane
        mLeftPane->setSize( (_sender->getSize().height-44) * aspect, _sender->getSize().height-44 );
        mRightPane->setCoord( mLeftPane->getPosition().left + (_sender->getSize().height-44) * aspect + 4,
                              mRightPane->getPosition().top,
                              _sender->getSize().width - 12 - (_sender->getSize().height-44) * aspect - 15,
                              _sender->getSize().height-44 );
        drawItems();
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
            if (mDragAndDrop->mDraggedFrom == this)
            {
                mWindowManager.getBookWindow()->setTakeButtonShow(false);
                mWindowManager.getScrollWindow()->setTakeButtonShow(false);
            }

            mDragAndDrop->mIsOnDragAndDrop = false;
            MyGUI::Gui::getInstance().destroyWidget(mDragAndDrop->mDraggedWidget);

            mWindowManager.setDragDrop(false);

            drawItems();

            // update selected weapon icon
            MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);
            MWWorld::ContainerStoreIterator weaponSlot = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (weaponSlot == invStore.end())
                mWindowManager.unsetSelectedWeapon();
            else
                mWindowManager.setSelectedWeapon(*weaponSlot, 100); /// \todo track weapon durability

        }
    }

    std::vector<MWWorld::Ptr> InventoryWindow::getEquippedItems()
    {
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(mPtr).getInventoryStore(mPtr);

        std::vector<MWWorld::Ptr> items;

        for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(slot);
            if (it != invStore.end())
            {
                items.push_back(*it);
            }
        }

        return items;
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
            if (toLower(it->getCellRef().refID) == "gold_001")
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
            && (type != typeid(ESM::Tool).name())
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
        MyGUI::ImageBox* baseWidget = mContainerWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 42, 42), MyGUI::Align::Default);
        baseWidget->detachFromWidget();
        baseWidget->attachToWidget(mDragAndDrop->mDragAndDropWidget);
        baseWidget->setUserData(newObject);
        mDragAndDrop->mDraggedWidget = baseWidget;
        ImageBox* image = baseWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
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
}
