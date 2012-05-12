#include "container.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"
#include "itemwidget.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwclass/container.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwsound/soundmanager.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <iostream>

#include <boost/lexical_cast.hpp>


using namespace MWGui;
using namespace Widgets;


ContainerBase::ContainerBase(WindowManager& parWindowManager,DragAndDrop* dragAndDrop,std::string guiFile)
    : WindowBase(guiFile, parWindowManager),
    mDragAndDrop(dragAndDrop),
    mContainer()
{
    getWidget(mContainerWidget, "Items");
    getWidget(mItemView, "ItemView");

    mContainerWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onContainerClicked);
}

ContainerBase::~ContainerBase()
{
}

void ContainerBase::onSelectedItem(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        mDragAndDrop->mIsOnDragAndDrop = true;
        _sender->detachFromWidget();
        _sender->attachToWidget(mDragAndDrop->mDragAndDropWidget);

        ItemWidget* item = static_cast<ItemWidget*>(_sender);

        int count = 0;
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);
        MWWorld::Ptr object;
        for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
        {
            count++;
            if(count == item->mPos)
            {
                mDragAndDrop->mStore.add(*iter);
                object = *iter;
                iter->getRefData().setCount(0);
                break;
            }
        }

        std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        _sender->setUserString("drag","on");
        mDragAndDrop->mDraggedWidget = _sender;
        mDragAndDrop->mContainerWindow = const_cast<MWGui::ContainerBase*>(this);
        drawItems();
    }
}

void ContainerBase::onContainerClicked(MyGUI::Widget* _sender)
{
    if(mDragAndDrop->mIsOnDragAndDrop) //drop widget here
    {
        ItemWidget* item = static_cast<ItemWidget*>(mDragAndDrop->mDraggedWidget);

        MWWorld::Ptr object = *item->getUserData<MWWorld::Ptr>();
        assert(object.getContainerStore() && "Item is not in a container!");

        std::string sound = MWWorld::Class::get(object).getDownSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);
        containerStore.add(*mDragAndDrop->mStore.begin());
        mDragAndDrop->mStore.clear();
        mDragAndDrop->mIsOnDragAndDrop = false;
        mDragAndDrop->mDraggedWidget->detachFromWidget();
        mDragAndDrop->mDraggedWidget->attachToWidget(mContainerWidget);
        mDragAndDrop->mDraggedWidget = 0;
        mDragAndDrop->mContainerWindow = 0;
        drawItems();
    }
}

void ContainerBase::setName(std::string contName)
{
    setText("_Main", contName);
    adjustWindowCaption();
}

void ContainerBase::open(MWWorld::Ptr container)
{
    mContainer = container;
    setName(MWWorld::Class::get(container).getName(container));
    drawItems();
    setVisible(true);
}

void ContainerBase::drawItems()
{
    while (mContainerWidget->getChildCount())
    {
        MyGUI::Gui::getInstance().destroyWidget(mContainerWidget->getChildAt(0));
    }
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

    int x = 4;
    int y = 4;
    int maxHeight = mItemView->getSize().height - 48;

    int index = 0;

    for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
    {
        index++;
        if(iter->getRefData().getCount() > 0)
        {
            std::string path = std::string("icons\\");
            path+=MWWorld::Class::get(*iter).getInventoryIcon(*iter);
            ItemWidget* image = mContainerWidget->createWidget<ItemWidget>("ImageBox", MyGUI::IntCoord(x, y, 32, 32), MyGUI::Align::Default);
            MyGUI::TextBox* text = image->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(14, 14, 18, 18), MyGUI::Align::Default, std::string("Label"));
            text->setTextAlign(MyGUI::Align::Right);
            text->setNeedMouseFocus(false);
            image->eventMouseButtonClick += MyGUI::newDelegate(this,&ContainerBase::onSelectedItem);
            image->setUserString("ToolTipType", "ItemPtr");
            image->setUserData(*iter);
            image->mPos = index;
            y += 36;
            if (y > maxHeight)
            {
                x += 36;
                y = 4;
            }

            if(iter->getRefData().getCount() > 1)
                text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()));

            int pos = path.rfind(".");
            path.erase(pos);
            path.append(".dds");
            image->setImageTexture(path);
        }
    }

    MyGUI::IntSize size = MyGUI::IntSize(std::max(mItemView->getSize().width, x), 2048);
    mItemView->setCanvasSize(size);
    mContainerWidget->setSize(size);
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
    : ContainerBase(parWindowManager, dragAndDrop, "openmw_container_window_layout.xml")
{
    getWidget(mTakeButton, "TakeButton");
    getWidget(mCloseButton, "CloseButton");

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
    int h = MyGUI::RenderManager::getInstance().getViewSize().height;
    setCoord(w-600,h-300,600,300);

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &ContainerWindow::onWindowResize);
}

ContainerWindow::~ContainerWindow()
{
}

void ContainerWindow::onWindowResize(MyGUI::Window* window)
{
    drawItems();
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

        for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
        {
            if(iter->getRefData().getCount() > 0)
            {
                playerStore.add(*iter);
            }
        }

        containerStore.clear();

        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}
