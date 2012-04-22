#include "container.hpp"

#include <iterator>
#include <algorithm>
#include "window_manager.hpp"
#include "widgets.hpp"

#include "../mwworld/environment.hpp"
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


using namespace MWGui;
using namespace Widgets;


ContainerWindow::ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment)
    : WindowBase("openmw_container_window_layout.xml", parWindowManager),
    mEnvironment(environment)
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

    setText("CloseButton","Close");
    setText("TakeButton","Take All");
    mContainerWidget->eventMouseItemActivate += MyGUI::newDelegate(this,&ContainerWindow::onSelectedItem);
}

ContainerWindow::ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,std::string guiFile)
    : WindowBase(guiFile, parWindowManager),
    mEnvironment(environment)
{
    setText("_Main", "Name of Container");
    //center();
    adjustWindowCaption();
    getWidget(mContainerWidget, "Items");
    //getWidget(takeButton, "TakeButton");
    //getWidget(closeButton, "CloseButton");

    //closeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onByeClicked);

    //setText("CloseButton","Close");
    //setText("TakeButton","Take All");
    mContainerWidget->eventMouseItemActivate += MyGUI::newDelegate(this,&ContainerWindow::onSelectedItem);
}
ContainerWindow::~ContainerWindow()
{
}

void ContainerWindow::setName(std::string contName)
{
    setText("_Main", contName);
}



void ContainerWindow::open(MWWorld::Ptr container)
{
    setName(MWWorld::Class::get(container).getName(container));
    //MWWorld::ContainerStore* containerStore = container.getContainerStore();

    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(container).getContainerStore(container);


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

    for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
    {
        std::string path = std::string("icons\\");
        path+=MWWorld::Class::get(*iter).getInventoryIcon(*iter);
        count++;

        MyGUI::ImageBox* image = mContainerWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(x, y, 32, 32), MyGUI::Align::Default);
        MyGUI::TextBox* text = mContainerWidget->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(x, y, 18, 18), MyGUI::Align::Default, std::string("Label"));

        x += 36;
        if(count % 20 == 0)
        {
            y += 36;
            x = 4;
            count = 0;
        }

        if(iter->getRefData().getCount() > 1)
            text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()));

        mContainerWidgets.push_back(image);

        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        //std::cout << path << std::endl;
        image->setImageTexture(path);
    }




    setVisible(true);
}

void ContainerWindow::Update()
{

}

void ContainerWindow::onByeClicked(MyGUI::Widget* _sender)
{
    mEnvironment.mWindowManager->setGuiMode(GM_Game);

    setVisible(false);
}

void ContainerWindow::onSelectedItem(MyGUI::ItemBox* _sender, size_t _index)
{
    std::cout << "selected!";
}
