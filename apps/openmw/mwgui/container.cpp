#include "container.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwclass/container.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwsound/soundmanager.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"
#include "countdialog.hpp"

using namespace MWGui;
using namespace Widgets;


ContainerBase::ContainerBase(DragAndDrop* dragAndDrop) :
    mDragAndDrop(dragAndDrop),
    mFilter(ContainerBase::Filter_All)
{
}

void ContainerBase::setWidgets(Widget* containerWidget, ScrollView* itemView)
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
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        mSelectedItem = _sender;

        MWWorld::Ptr object = (*_sender->getUserData<MWWorld::Ptr>());
        int count = object.getRefData().getCount();

        if (MyGUI::InputManager::getInstance().isShiftPressed() || count == 1)
        {
            onSelectedItemImpl(_sender, count);
        }
        else if (MyGUI::InputManager::getInstance().isControlPressed())
        {
            onSelectedItemImpl(_sender, 1);
        }
        else
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            dialog->open(MWWorld::Class::get(object).getName(object), count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerBase::onSelectedItemImpl);
        }
    }
    else
        onContainerClicked(mContainerWidget);
}

void ContainerBase::onSelectedItemImpl(MyGUI::Widget* _sender, int count)
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

    mDragAndDrop->mWasInInventory = isInventory();

    drawItems();

    MWBase::Environment::get().getWindowManager()->setDragDrop(true);
}

void ContainerBase::onContainerClicked(MyGUI::Widget* _sender)
{
    if(mDragAndDrop->mIsOnDragAndDrop) //drop widget here
    {
        MWWorld::Ptr object = *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>();
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

        if (mDragAndDrop->mDraggedFrom != this)
        {
            assert(object.getContainerStore() && "Item is not in a container!");


            // check that we don't exceed the allowed weight (only for containers, not for inventory)
            if (!isInventory())
            {
                float capacity = MWWorld::Class::get(mContainer).getCapacity(mContainer);

                // try adding the item, and if weight is exceeded, just remove it again.
                int origCount = object.getRefData().getCount();
                object.getRefData().setCount(mDragAndDrop->mDraggedCount);
                MWWorld::ContainerStoreIterator it = containerStore.add(object);

                float curWeight = MWWorld::Class::get(mContainer).getEncumbrance(mContainer);
                if (curWeight > capacity)
                {
                    it->getRefData().setCount(0);
                    object.getRefData().setCount(origCount);
                    // user notification
                    MWBase::Environment::get().getWindowManager()->
                        messageBox(MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sContentsMessage3")->str, std::vector<std::string>());
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
                int origCount = object.getRefData().getCount();
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
    mContainer = container;
    drawItems();
}

void ContainerBase::drawItems()
{
    while (mContainerWidget->getChildCount())
    {
        MyGUI::Gui::getInstance().destroyWidget(mContainerWidget->getChildAt(0));
    }
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

    int x = 0;
    int y = 0;
    int maxHeight = mItemView->getSize().height - 58;

    int index = 0;


    bool onlyMagic = false;
    int categories;
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
                    + MWWorld::ContainerStore::Type_Apparatus;
    }

    /// \todo performance improvement: don't create/destroy all the widgets everytime the container window changes size, only reposition them

    std::vector< std::pair<MWWorld::Ptr, ItemState> > items;

    std::vector<MWWorld::Ptr> equippedItems = getEquippedItems();

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
        std::vector<MWWorld::Ptr>::iterator found = std::find(unwantedItems.begin(), unwantedItems.end(), *it);
        assert(found != unwantedItems.end());
        equippedItems.erase(found);
    }
    // and add the items that are left (= have the correct category)
    for (std::vector<MWWorld::Ptr>::const_iterator it=equippedItems.begin();
        it != equippedItems.end(); ++it)
    {
        items.push_back( std::make_pair(*it, ItemState_Equipped) );
    }

    // now add the regular items
    for (MWWorld::ContainerStoreIterator iter (containerStore.begin(categories)); iter!=containerStore.end(); ++iter)
    {
        /// \todo sorting
        if (std::find(equippedItems.begin(), equippedItems.end(), *iter) == equippedItems.end())
            items.push_back( std::make_pair(*iter, ItemState_Normal) );
    }

    for (std::vector< std::pair<MWWorld::Ptr, ItemState> >::const_iterator it=items.begin();
        it != items.end(); ++it)
    {
        index++;
        const MWWorld::Ptr* iter = &((*it).first);

        int displayCount = iter->getRefData().getCount();
        if (mDragAndDrop->mIsOnDragAndDrop && *iter == *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>())
        {
            displayCount -= mDragAndDrop->mDraggedCount;
        }
        if(displayCount > 0 && !(onlyMagic && MWWorld::Class::get(*iter).getEnchantment(*iter) == "" && iter->getTypeName() != typeid(ESM::Potion).name()))
        {
            std::string path = std::string("icons\\");
            path+=MWWorld::Class::get(*iter).getInventoryIcon(*iter);

            // background widget (for the "equipped" frame and magic item background image)
            bool isMagic = (MWWorld::Class::get(*iter).getEnchantment(*iter) != "");
            MyGUI::ImageBox* backgroundWidget = mContainerWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(x, y, 42, 42), MyGUI::Align::Default);
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
            backgroundTex += ".dds";

            backgroundWidget->setImageTexture(backgroundTex);
            backgroundWidget->setProperty("ImageCoord", "0 0 42 42");
            backgroundWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onSelectedItem);
            backgroundWidget->eventMouseWheel += MyGUI::newDelegate(this, &ContainerBase::onMouseWheel);

            // image
            ImageBox* image = backgroundWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
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

            y += 42;
            if (y > maxHeight)
            {
                x += 42;
                y = 0;
            }

            text->setCaption(getCountString(displayCount));
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
        return boost::lexical_cast<std::string>(count/1000.f) + "k";
    else
        return boost::lexical_cast<std::string>(count);
}

void ContainerBase::Update()
{
    if(mDragAndDrop->mIsOnDragAndDrop)
    {
        if(mDragAndDrop->mDraggedWidget)
            mDragAndDrop->mDraggedWidget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
        else mDragAndDrop->mIsOnDragAndDrop = false; //If this happens, there is a bug.
    }
}

// ------------------------------------------------------------------------------------------------

ContainerWindow::ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
    : ContainerBase(dragAndDrop)
    , WindowBase("openmw_container_window_layout.xml", parWindowManager)
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

    setText("CloseButton", MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sClose")->str);
    setText("TakeButton", MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTakeAll")->str);

    // adjust buttons size to fit text
    int closeButtonWidth = mCloseButton->getTextSize().width+24;
    int takeButtonWidth = mTakeButton->getTextSize().width+24;
    mCloseButton->setCoord(600-20-closeButtonWidth, mCloseButton->getCoord().top, closeButtonWidth, mCloseButton->getCoord().height);
    mTakeButton->setCoord(600-20-closeButtonWidth-takeButtonWidth-8, mTakeButton->getCoord().top, takeButtonWidth, mTakeButton->getCoord().height);

    int w = MyGUI::RenderManager::getInstance().getViewSize().width;
    //int h = MyGUI::RenderManager::getInstance().getViewSize().height;

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &ContainerWindow::onWindowResize);

    setCoord(w-600,0,600,300);
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
}

void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}

void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        // transfer everything into the player's inventory
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

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

        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}
