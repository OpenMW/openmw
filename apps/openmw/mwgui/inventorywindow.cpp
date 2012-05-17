#include "inventorywindow.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "../mwclass/container.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/player.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/manualref.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"
#include "bookwindow.hpp"
#include "scrollwindow.hpp"

namespace MWGui
{

    InventoryWindow::InventoryWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
        : ContainerBase(dragAndDrop)
        , WindowPinnableBase("openmw_inventory_window_layout.xml", parWindowManager)
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

        mFilterAll->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sAllTab")->str);
        mFilterWeapon->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sWeaponTab")->str);
        mFilterApparel->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sApparelTab")->str);
        mFilterMagic->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sMagicTab")->str);
        mFilterMisc->setCaption (MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sMiscTab")->str);

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

        setCoord(0, 342, 600, 258);
    }

    void InventoryWindow::openInventory()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        openContainer(player);

        onWindowResize(static_cast<MyGUI::Window*>(mMainWidget));

        updateEncumbranceBar();
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
                MWWorld::InventoryStore& invStore = static_cast<MWWorld::InventoryStore&>(MWWorld::Class::get(mContainer).getContainerStore(mContainer));
                MWWorld::ContainerStoreIterator it = invStore.begin();

                int origCount = ptr.getRefData().getCount();
                ptr.getRefData().setCount(origCount - mDragAndDrop->mDraggedCount);
                it = invStore.add(ptr);
                (*it).getRefData().setCount(mDragAndDrop->mDraggedCount);
                ptr = *it;
            }

            /// \todo scripts

            boost::shared_ptr<MWWorld::Action> action = MWWorld::Class::get(ptr).use(ptr);
            
            action->execute();

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
        }
    }

    std::vector<MWWorld::Ptr> InventoryWindow::getEquippedItems()
    {
        MWWorld::InventoryStore& invStore = static_cast<MWWorld::InventoryStore&>(MWWorld::Class::get(mContainer).getContainerStore(mContainer));

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
        MWWorld::InventoryStore& invStore = static_cast<MWWorld::InventoryStore&>(MWWorld::Class::get(mContainer).getContainerStore(mContainer));

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

    void InventoryWindow::notifyContentChanged()
    {
    }

    void InventoryWindow::Update()
    {
        updateEncumbranceBar();

        ContainerBase::Update();
    }
}
