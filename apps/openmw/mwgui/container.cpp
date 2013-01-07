#include "container.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "../mwclass/container.hpp"

#include "widgets.hpp"
#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "inventorywindow.hpp"

using namespace MWGui;
using namespace Widgets;


namespace
{
    bool compareType(std::string type1, std::string type2)
    {
        // this defines the sorting order of types. types that are first in the vector, appear before other types.
        std::vector<std::string> mapping;
        mapping.push_back( typeid(ESM::Weapon).name() );
        mapping.push_back( typeid(ESM::Armor).name() );
        mapping.push_back( typeid(ESM::Clothing).name() );
        mapping.push_back( typeid(ESM::Potion).name() );
        mapping.push_back( typeid(ESM::Ingredient).name() );
        mapping.push_back( typeid(ESM::Apparatus).name() );
        mapping.push_back( typeid(ESM::Book).name() );
        mapping.push_back( typeid(ESM::Light).name() );
        mapping.push_back( typeid(ESM::Miscellaneous).name() );
        mapping.push_back( typeid(ESM::Tool).name() );
        mapping.push_back( typeid(ESM::Repair).name() );
        mapping.push_back( typeid(ESM::Probe).name() );

        assert( std::find(mapping.begin(), mapping.end(), type1) != mapping.end() );
        assert( std::find(mapping.begin(), mapping.end(), type2) != mapping.end() );

        return std::find(mapping.begin(), mapping.end(), type1) < std::find(mapping.begin(), mapping.end(), type2);
    }

    bool sortItems(MWWorld::Ptr left, MWWorld::Ptr right)
    {
        if (left.getTypeName() == right.getTypeName())
        {
            int cmp = MWWorld::Class::get(left).getName(left).compare(
                        MWWorld::Class::get(right).getName(right));
            return cmp < 0;
        }
        else
        {
            return compareType(left.getTypeName(), right.getTypeName());
        }
    }
}


ContainerBase::ContainerBase(DragAndDrop* dragAndDrop) :
    mDragAndDrop(dragAndDrop),
    mFilter(ContainerBase::Filter_All)
{
}

void ContainerBase::setWidgets(MyGUI::Widget* containerWidget, MyGUI::ScrollView* itemView)
{
    mContainerWidget = containerWidget;
    mItemView = itemView;

    mContainerWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onContainerClicked);
    mContainerWidget->eventMouseWheel += MyGUI::newDelegate(this, &ContainerWindow::onMouseWheel);
}

ContainerBase::~ContainerBase()
{
}

void ContainerBase::onSelectedItem(MyGUI::Widget* _sender)
{
    mSelectedItem = _sender;

    if (mDragAndDrop && !isTrading())
    {
        if(!mDragAndDrop->mIsOnDragAndDrop)
        {
            MWWorld::Ptr object = (*_sender->getUserData<MWWorld::Ptr>());
            int count = object.getRefData().getCount();

            if (MyGUI::InputManager::getInstance().isShiftPressed() || count == 1)
            {
                startDragItem(_sender, count);
            }
            else if (MyGUI::InputManager::getInstance().isControlPressed())
            {
                startDragItem(_sender, 1);
            }
            else
            {
                std::string message = "#{sTake}";
                CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
                dialog->open(MWWorld::Class::get(object).getName(object), message, count);
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerBase::startDragItem);
            }
        }
        else
            onContainerClicked(mContainerWidget);
    }
    else if (isTrading())
    {
        MWWorld::Ptr object = (*_sender->getUserData<MWWorld::Ptr>());
        int count = object.getRefData().getCount();

        if (isInventory())
        {
            const MWWorld::Store<ESM::GameSetting> &gmst =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

            // the player is trying to sell an item, check if the merchant accepts it
            // also, don't allow selling gold (let's be better than Morrowind at this, can we?)
            if (!MWBase::Environment::get().getWindowManager()->getTradeWindow()->npcAcceptsItem(object) ||
                MWWorld::Class::get(object).getName(object) == gmst.find("sGold")->getString())
            {
                // user notification "i don't buy this item"
                MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sBarterDialog4}", std::vector<std::string>());
                return;
            }
        }

        bool buying = isTradeWindow(); // buying or selling?
        std::string message = buying ? "#{sQuanityMenuMessage02}" : "#{sQuanityMenuMessage01}";

        if (std::find(mBoughtItems.begin(), mBoughtItems.end(), object) != mBoughtItems.end())
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed() || count == 1)
            {
                sellAlreadyBoughtItem(NULL, count);
            }
            else if (MyGUI::InputManager::getInstance().isControlPressed())
            {
                sellAlreadyBoughtItem(NULL, 1);
            }
            else
            {
                CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
                dialog->open(MWWorld::Class::get(object).getName(object), message, count);
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerBase::sellAlreadyBoughtItem);
            }
        }
        else
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed() || count == 1)
            {
                sellItem(NULL, count);
            }
            else if (MyGUI::InputManager::getInstance().isControlPressed())
            {
                sellItem(NULL, 1);
            }
            else
            {
                CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
                dialog->open(MWWorld::Class::get(object).getName(object), message, count);
                dialog->eventOkClicked.clear();
                dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerBase::sellItem);
            }
        }
    }
    else
    {
        onSelectedItemImpl(*_sender->getUserData<MWWorld::Ptr>());
    }
}

void ContainerBase::sellAlreadyBoughtItem(MyGUI::Widget* _sender, int count)
{
    MWWorld::Ptr object = *mSelectedItem->getUserData<MWWorld::Ptr>();

    if (isInventory())
    {
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->addItem(object, count);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->sellToNpc(object, count, true);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->drawItems();
    }
    else
    {
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->addItem(object, count);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->buyFromNpc(object, count, true);
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->drawItems();
    }

    std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

    drawItems();
}

void ContainerBase::sellItem(MyGUI::Widget* _sender, int count)
{
    MWWorld::Ptr object = *mSelectedItem->getUserData<MWWorld::Ptr>();

    if (isInventory())
    {
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->addBarteredItem(object, count);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->sellToNpc(object, count, false);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->drawItems();
    }
    else
    {
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->addBarteredItem(object, count);
        MWBase::Environment::get().getWindowManager()->getTradeWindow()->buyFromNpc(object, count, false);
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->drawItems();
    }

    std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

    drawItems();
}

void ContainerBase::startDragItem(MyGUI::Widget* _sender, int count)
{
    mDragAndDrop->mIsOnDragAndDrop = true;
    mSelectedItem->detachFromWidget();
    mSelectedItem->attachToWidget(mDragAndDrop->mDragAndDropWidget);

    MWWorld::Ptr object = *mSelectedItem->getUserData<MWWorld::Ptr>();
    _unequipItem(object);

    mDragAndDrop->mDraggedCount = count;

    mDragAndDrop->mDraggedFrom = this;

    std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

    mDragAndDrop->mDraggedWidget = mSelectedItem;
    static_cast<MyGUI::ImageBox*>(mSelectedItem)->setImageTexture(""); // remove the background texture (not visible during drag)
    static_cast<MyGUI::TextBox*>(mSelectedItem->getChildAt(0)->getChildAt(0))->setCaption(
        getCountString(mDragAndDrop->mDraggedCount));

    drawItems();

    MWBase::Environment::get().getWindowManager()->setDragDrop(true);
}

void ContainerBase::onContainerClicked(MyGUI::Widget* _sender)
{
    if (mDragAndDrop == NULL) return;

    if(mDragAndDrop->mIsOnDragAndDrop) //drop item here
    {
        MWWorld::Ptr object = *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>();
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

        if (mDragAndDrop->mDraggedFrom != this)
        {
            assert(object.getContainerStore() && "Item is not in a container!");

            // check the container's Organic flag (if this is a container). container with Organic flag doesn't allow putting items inside
            if (mPtr.getTypeName() == typeid(ESM::Container).name())
            {
                MWWorld::LiveCellRef<ESM::Container>* ref = mPtr.get<ESM::Container>();
                if (ref->mBase->mFlags & ESM::Container::Organic)
                {
                    // user notification
                    MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sContentsMessage2}", std::vector<std::string>());

                    return;
                }
            }

            int origCount = object.getRefData().getCount();

            // check that we don't exceed the allowed weight (only for containers, not for inventory)
            if (!isInventory())
            {
                float capacity = MWWorld::Class::get(mPtr).getCapacity(mPtr);

                // try adding the item, and if weight is exceeded, just remove it again.
                object.getRefData().setCount(mDragAndDrop->mDraggedCount);
                MWWorld::ContainerStoreIterator it = containerStore.add(object);

                float curWeight = MWWorld::Class::get(mPtr).getEncumbrance(mPtr);
                if (curWeight > capacity)
                {
                    it->getRefData().setCount(0);
                    object.getRefData().setCount(origCount);
                    // user notification
                    MWBase::Environment::get().getWindowManager()->
                        messageBox("#{sContentsMessage3}", std::vector<std::string>());

                    return;
                }
                else
                {
                    object.getRefData().setCount(origCount - mDragAndDrop->mDraggedCount);
                }
                std::cout << "container weight " << curWeight << "/" << capacity << std::endl;
            }
            else
            {
                object.getRefData().setCount (mDragAndDrop->mDraggedCount);
                containerStore.add(object);
                object.getRefData().setCount (origCount - mDragAndDrop->mDraggedCount);
            }
        }

        mDragAndDrop->mIsOnDragAndDrop = false;
        MyGUI::Gui::getInstance().destroyWidget(mDragAndDrop->mDraggedWidget);
        drawItems();
        mDragAndDrop->mDraggedFrom->drawItems();

        MWBase::Environment::get().getWindowManager()->setDragDrop(false);

        std::string sound = MWWorld::Class::get(object).getDownSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
    }
}

void ContainerBase::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mItemView->getViewOffset().left + _rel*0.3 > 0)
        mItemView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mItemView->setViewOffset(MyGUI::IntPoint(mItemView->getViewOffset().left + _rel*0.3, 0));
}

void ContainerBase::setFilter(ContainerBase::Filter filter)
{
    mFilter = filter;
    drawItems();
}

void ContainerBase::openContainer(MWWorld::Ptr container)
{
    mPtr = container;
}

void ContainerBase::drawItems()
{
    while (mContainerWidget->getChildCount())
    {
        MyGUI::Gui::getInstance().destroyWidget(mContainerWidget->getChildAt(0));
    }
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

    int x = 0;
    int y = 0;
    int maxHeight = mItemView->getSize().height - 58;

    bool onlyMagic = false;
    int categories = 0;
    if (mFilter == Filter_All)
        categories = MWWorld::ContainerStore::Type_All;
    else if (mFilter == Filter_Weapon)
        categories = MWWorld::ContainerStore::Type_Weapon;
    else if (mFilter == Filter_Apparel)
        categories = MWWorld::ContainerStore::Type_Clothing + MWWorld::ContainerStore::Type_Armor;
    else if (mFilter == Filter_Magic)
    {
        categories = MWWorld::ContainerStore::Type_Clothing + MWWorld::ContainerStore::Type_Armor
                    + MWWorld::ContainerStore::Type_Weapon + MWWorld::ContainerStore::Type_Book
                    + MWWorld::ContainerStore::Type_Potion;
        onlyMagic = true;
    }
    else if (mFilter == Filter_Misc)
    {
        categories = MWWorld::ContainerStore::Type_Miscellaneous + MWWorld::ContainerStore::Type_Book
                    + MWWorld::ContainerStore::Type_Ingredient + MWWorld::ContainerStore::Type_Repair
                    + MWWorld::ContainerStore::Type_Lockpick + MWWorld::ContainerStore::Type_Light
                    + MWWorld::ContainerStore::Type_Apparatus + MWWorld::ContainerStore::Type_Probe;
    }
    else if (mFilter == Filter_Ingredients)
        categories = MWWorld::ContainerStore::Type_Ingredient;

    /// \todo performance improvement: don't create/destroy all the widgets everytime the container window changes size, only reposition them

    std::vector< std::pair<MWWorld::Ptr, ItemState> > items;

    std::vector<MWWorld::Ptr> equippedItems = getEquippedItems();

    // add bought items (always at the beginning)
    std::vector<MWWorld::Ptr> boughtItems;
    for (MWWorld::ContainerStoreIterator it (mBoughtItems.begin()); it!=mBoughtItems.end(); ++it)
    {
        boughtItems.push_back(*it);
    }
    std::sort(boughtItems.begin(), boughtItems.end(), sortItems);

    for (std::vector<MWWorld::Ptr>::iterator it=boughtItems.begin();
        it != boughtItems.end(); ++it)
    {
        items.push_back( std::make_pair(*it, ItemState_Barter) );
    }

    // filter out the equipped items of categories we don't want
    std::vector<MWWorld::Ptr> unwantedItems = equippedItems;
    for (MWWorld::ContainerStoreIterator iter (containerStore.begin(categories)); iter!=containerStore.end(); ++iter)
    {
        std::vector<MWWorld::Ptr>::iterator found = std::find(unwantedItems.begin(), unwantedItems.end(), *iter);
        if (found != unwantedItems.end())
        {
            unwantedItems.erase(found);
        }
    }
    // now erase everything that's still in unwantedItems.
    for (std::vector<MWWorld::Ptr>::iterator it=unwantedItems.begin();
        it != unwantedItems.end(); ++it)
    {
        std::vector<MWWorld::Ptr>::iterator found = std::find(equippedItems.begin(), equippedItems.end(), *it);
        assert(found != equippedItems.end());
        equippedItems.erase(found);
    }
    // and add the items that are left (= have the correct category)
    if (!ignoreEquippedItems())
    {
        for (std::vector<MWWorld::Ptr>::const_iterator it=equippedItems.begin();
            it != equippedItems.end(); ++it)
        {
            items.push_back( std::make_pair(*it, ItemState_Equipped) );
        }
    }

    std::vector<MWWorld::Ptr> ignoreItems = itemsToIgnore();

    // now add the regular items
    std::vector<MWWorld::Ptr> regularItems;
    for (MWWorld::ContainerStoreIterator iter (containerStore.begin(categories)); iter!=containerStore.end(); ++iter)
    {
        if (std::find(equippedItems.begin(), equippedItems.end(), *iter) == equippedItems.end()
            && std::find(ignoreItems.begin(), ignoreItems.end(), *iter) == ignoreItems.end()
            && std::find(mBoughtItems.begin(), mBoughtItems.end(), *iter) == mBoughtItems.end())
            regularItems.push_back(*iter);
    }

    // sort them and add
    std::sort(regularItems.begin(), regularItems.end(), sortItems);
    for (std::vector<MWWorld::Ptr>::const_iterator it=regularItems.begin(); it!=regularItems.end(); ++it)
    {
        items.push_back( std::make_pair(*it, ItemState_Normal) );
    }

    for (std::vector< std::pair<MWWorld::Ptr, ItemState> >::const_iterator it=items.begin();
        it != items.end(); ++it)
    {
        const MWWorld::Ptr* iter = &((*it).first);

        int displayCount = iter->getRefData().getCount();
        if (mDragAndDrop != NULL && mDragAndDrop->mIsOnDragAndDrop && *iter == *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>())
        {
            displayCount -= mDragAndDrop->mDraggedCount;
        }
        if(displayCount > 0 && !(onlyMagic && it->second != ItemState_Barter && MWWorld::Class::get(*iter).getEnchantment(*iter) == "" && iter->getTypeName() != typeid(ESM::Potion).name()))
        {
            std::string path = std::string("icons\\");
            path += MWWorld::Class::get(*iter).getInventoryIcon(*iter);

            // background widget (for the "equipped" frame and magic item background image)
            bool isMagic = (MWWorld::Class::get(*iter).getEnchantment(*iter) != "");
            MyGUI::ImageBox* backgroundWidget = mContainerWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(x, y, 42, 42), MyGUI::Align::Default);
            backgroundWidget->setUserString("ToolTipType", "ItemPtr");
            backgroundWidget->setUserData(*iter);

            std::string backgroundTex = "textures\\menu_icon";
            if (isMagic)
                backgroundTex += "_magic";
            if (it->second == ItemState_Normal)
            {
                if (!isMagic)
                    backgroundTex = "";
            }
            else if (it->second == ItemState_Equipped)
            {
                backgroundTex += "_equip";
            }
            else if (it->second == ItemState_Barter)
            {
                backgroundTex += "_barter";
            }
            if (backgroundTex != "")
                backgroundTex += ".dds";

            backgroundWidget->setImageTexture(backgroundTex);
            if (it->second == ItemState_Barter && !isMagic)
                backgroundWidget->setProperty("ImageCoord", "2 2 42 42");
            else
                backgroundWidget->setProperty("ImageCoord", "0 0 42 42");
            backgroundWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onSelectedItem);
            backgroundWidget->eventMouseWheel += MyGUI::newDelegate(this, &ContainerBase::onMouseWheel);

            // image
            MyGUI::ImageBox* image = backgroundWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
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
            text->setCaption(getCountString(displayCount));

            y += 42;
            if (y > maxHeight)
            {
                x += 42;
                y = 0;
            }

        }
    }

    MyGUI::IntSize size = MyGUI::IntSize(std::max(mItemView->getSize().width, x+42), mItemView->getSize().height);
    mItemView->setCanvasSize(size);
    mContainerWidget->setSize(size);

    notifyContentChanged();
}

std::string ContainerBase::getCountString(const int count)
{
    if (count == 1)
        return "";
    if (count > 9999)
        return boost::lexical_cast<std::string>(int(count/1000.f)) + "k";
    else
        return boost::lexical_cast<std::string>(count);
}

void ContainerBase::addBarteredItem(MWWorld::Ptr item, int count)
{
    int origCount = item.getRefData().getCount();
    item.getRefData().setCount(count);
    MWWorld::ContainerStoreIterator it = mBoughtItems.add(item);
    item.getRefData().setCount(origCount - count);
}

void ContainerBase::addItem(MWWorld::Ptr item, int count)
{
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

    int origCount = item.getRefData().getCount();

    item.getRefData().setCount(count);
    MWWorld::ContainerStoreIterator it = containerStore.add(item);

    item.getRefData().setCount(origCount - count);
}

void ContainerBase::transferBoughtItems()
{
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

    for (MWWorld::ContainerStoreIterator it(mBoughtItems.begin()); it != mBoughtItems.end(); ++it)
    {
        containerStore.add(*it);
    }
}

void ContainerBase::returnBoughtItems(MWWorld::ContainerStore& store)
{
    for (MWWorld::ContainerStoreIterator it(mBoughtItems.begin()); it != mBoughtItems.end(); ++it)
    {
        store.add(*it);
    }
}

MWWorld::ContainerStore& ContainerBase::getContainerStore()
{
    MWWorld::ContainerStore& store = MWWorld::Class::get(mPtr).getContainerStore(mPtr);
    return store;
}

// ------------------------------------------------------------------------------------------------

ContainerWindow::ContainerWindow(MWBase::WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
    : ContainerBase(dragAndDrop)
    , WindowBase("openmw_container_window.layout", parWindowManager)
{
    getWidget(mTakeButton, "TakeButton");
    getWidget(mCloseButton, "CloseButton");

    MyGUI::ScrollView* itemView;
    MyGUI::Widget* containerWidget;
    getWidget(containerWidget, "Items");
    getWidget(itemView, "ItemView");
    setWidgets(containerWidget, itemView);

    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
    mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &ContainerWindow::onWindowResize);

    setCoord(200,0,600,300);
}

ContainerWindow::~ContainerWindow()
{
}

void ContainerWindow::onWindowResize(MyGUI::Window* window)
{
    drawItems();
}

void ContainerWindow::open(MWWorld::Ptr container)
{
    openContainer(container);
    setTitle(MWWorld::Class::get(container).getName(container));
    drawItems();
}

void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
{
    if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }
}

void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
{
    if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
    {
        // transfer everything into the player's inventory
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mPtr).getContainerStore(mPtr);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::ContainerStore& playerStore = MWWorld::Class::get(player).getContainerStore(player);

        int i=0;
        for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
        {
            playerStore.add(*iter);

            if (i==0)
            {
                // play the sound of the first object
                std::string sound = MWWorld::Class::get(*iter).getUpSoundId(*iter);
                MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
            }

            ++i;
        }

        containerStore.clear();

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }
}

void ContainerWindow::onReferenceUnavailable()
{
    MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
}
