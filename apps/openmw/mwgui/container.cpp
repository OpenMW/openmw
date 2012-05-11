#include "container.hpp"

#include <iterator>
#include <algorithm>
#include "window_manager.hpp"
#include "widgets.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/manualref.hpp"
#include <cmath>
#include <algorithm>
#include <iterator>

#include <assert.h>
#include <iostream>
#include "../mwclass/container.hpp"
#include "../mwworld/containerstore.hpp"
#include <boost/lexical_cast.hpp>
#include "../mwworld/class.hpp"
#include "../mwinput/inputmanager.hpp"
#include "itemwidget.hpp"


using namespace MWGui;
using namespace Widgets;


ContainerWindow::ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
    : WindowBase("openmw_container_window_layout.xml", parWindowManager),
    mDragAndDrop(dragAndDrop),
    mContainer()
{
    setText("_Main", "Name of Container");

    int w = MyGUI::RenderManager::getInstance().getViewSize().width;
    int h = MyGUI::RenderManager::getInstance().getViewSize().height;
    setCoord(w-600,h-300,600,300);
    //center();
    adjustWindowCaption();

    getWidget(mContainerWidget, "Items");
    getWidget(takeButton, "TakeButton");
    getWidget(closeButton, "CloseButton");

    closeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onByeClicked);
    mContainerWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onContainerClicked);
    setText("CloseButton","Close");
    setText("TakeButton","Take All");

    //mContainerWidget->eventMouseItemActivate += MyGUI::newDelegate(this,&ContainerWindow::onSelectedItem);
}

ContainerWindow::ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop,std::string guiFile)
    : WindowBase(guiFile, parWindowManager),
    mDragAndDrop(dragAndDrop),
    mContainer()
{
    setText("_Main", "Name of Container");
    //center();
    adjustWindowCaption();
    getWidget(mContainerWidget, "Items");
    mContainerWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onContainerClicked);
    //getWidget(takeButton, "TakeButton");
    //getWidget(closeButton, "CloseButton");

    //closeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onByeClicked);

    //setText("CloseButton","Close");
    //setText("TakeButton","Take All");
    //mContainerWidget->eventMouseItemActivate += MyGUI::newDelegate(this,&ContainerWindow::onSelectedItem);
}
ContainerWindow::~ContainerWindow()
{
}

void ContainerWindow::setName(std::string contName)
{
    setText("_Main", contName);
}



void ContainerWindow::open(MWWorld::Ptr& container)
{
    mContainer = container;
    setName(MWWorld::Class::get(container).getName(container));
    //MWWorld::ContainerStore* containerStore = container.getContainerStore();
    drawItems();
    setVisible(true);
}

void ContainerWindow::drawItems()
{
    MyGUI::Gui::getInstance().destroyWidgets(mContainerWidget->getEnumerator());
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);
    //mContainerWidget->

    /*MWWorld::ManualRef furRef (mWindowManager.getStore(), "fur_cuirass");
    furRef.getPtr().getRefData().setCount (5);
    MWWorld::ManualRef bukkitRef (mWindowManager.getStore(), "misc_com_bucket_01");
    MWWorld::ManualRef broomRef (mWindowManager.getStore(), "misc_com_broom_01");
    MWWorld::ManualRef goldRef (mWindowManager.getStore(), "gold_100");

    containerStore.add(furRef.getPtr());
    containerStore.add(furRef.getPtr());
    containerStore.add(furRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(broomRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(bukkitRef.getPtr());
    containerStore.add(goldRef.getPtr());*/


    // ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref = iter->get<ESM::Armor>();

    int x = 4;
    int y = 4;
    int count = 0;
    int index = 0;

    for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
    {
        index++;
        if(iter->getRefData().getCount() > 0)
        {
            count++;
            std::string path = std::string("icons\\");
            path+=MWWorld::Class::get(*iter).getInventoryIcon(*iter);
            ItemWidget* image = mContainerWidget->createWidget<ItemWidget>("ImageBox", MyGUI::IntCoord(x, y, 32, 32), MyGUI::Align::Default);
            MyGUI::TextBox* text = image->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(x, y, 18, 18), MyGUI::Align::Default, std::string("Label"));
            image->eventMouseButtonClick += MyGUI::newDelegate(this,&ContainerWindow::onSelectedItem);
            image->mPos = index;
            image->mPtr = *iter;
            //image->eventMouseMove += MyGUI::newDelegate(this,&ContainerWindow::onMouseMove);
            x += 36;
            if(count % 20 == 0)
            {
            y += 36;
            x = 4;
            count = 0;
            }

            if(iter->getRefData().getCount() > 1)
                text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()));

            int pos = path.rfind(".");
            path.erase(pos);
            path.append(".dds");
            image->setImageTexture(path);
        }
    } 
}

void ContainerWindow::Update()
{
    if(mDragAndDrop->mIsOnDragAndDrop)
    {
        if(mDragAndDrop->mDraggedWidget)
            mDragAndDrop->mDraggedWidget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
        else mDragAndDrop->mIsOnDragAndDrop = false; //If this happens, there is a bug.
    }
}

void ContainerWindow::onByeClicked(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}

void ContainerWindow::onSelectedItem(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        mDragAndDrop->mIsOnDragAndDrop = true;
        _sender->detachFromWidget();
        _sender->attachToWidget(mDragAndDrop->mDragAndDropWidget);

        ItemWidget* item = static_cast<ItemWidget*>(_sender);

        int count = 0;
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);
        for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
        {
            count++;
            if(count == item->mPos)
            {
                iter->getRefData().setCount(0);
                break;
            }
        }
        //containerStore.
        //std::cout << mContainerWidget->getParent()->getParent()->getName();
        _sender->setUserString("drag","on");
        mDragAndDrop->mDraggedWidget = _sender;
        mDragAndDrop->mContainerWindow = const_cast<MWGui::ContainerWindow*>(this);
        drawItems();
        std::cout << "selected!";
    }
}

void ContainerWindow::onMouseMove(MyGUI::Widget* _sender, int _left, int _top)
{
    /*if(_sender->getUserString("drag") == "on")
    {
    _sender->setPosition(_left,_top);

    }*/
}

void ContainerWindow::onContainerClicked(MyGUI::Widget* _sender)
{
    std::cout << "container clicked";
    if(mDragAndDrop->mIsOnDragAndDrop) //drop widget here
    {
        ItemWidget* item = static_cast<ItemWidget*>(mDragAndDrop->mDraggedWidget);
        std::cout << item->mPos << item->mPtr.getTypeName();
        if(item->mPtr.getContainerStore() == 0) std::cout << "nocontainer!";
        std::cout << item->mPtr.getContainerStore()->getType(item->mPtr);
        MWWorld::Ptr ptr = item->mPtr;
        //MWWorld::World
        //mContainer.getContainerStore()->add(item->mPtr);
        mDragAndDrop->mIsOnDragAndDrop = false;
        mDragAndDrop->mDraggedWidget->detachFromWidget();
        mDragAndDrop->mDraggedWidget->attachToWidget(mContainerWidget);
        mDragAndDrop->mDraggedWidget = 0;
        mDragAndDrop->mContainerWindow = 0;
    }
}