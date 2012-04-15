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


using namespace MWGui;
using namespace Widgets;


ContainerWindow::ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment)
    : WindowBase("openmw_container_window_layout.xml", parWindowManager),
    mEnvironment(environment)
{
    setText("_Main", "Name of Container");
    center();

    getWidget(containerWidget, "Items");
    getWidget(takeButton, "TakeButton");
    getWidget(closeButton, "CloseButton");

    setText("CloseButton","Close");
    setText("TakeButton","Take All");
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
    setName(MWWorld::Class::get(container).getName(container));
    //MWWorld::ContainerStore* containerStore = container.getContainerStore();

    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(container).getContainerStore(container);


    MWWorld::ManualRef furRef (mWindowManager.getStore(), "fur_cuirass");
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
    containerStore.add(goldRef.getPtr());



    // ESMS::LiveCellRef<ESM::Armor, MWWorld::RefData> *ref = iter->get<ESM::Armor>();


    int x = 4;
    int y = 4;
    int count = 0;

    for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
    {
        std::string path = std::string("icons\\");


        //path += iter.getInventoryIcon();
        switch (iter.getType())
        {

        case MWWorld::ContainerStore::Type_Potion:
            path += iter->get<ESM::Potion>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Apparatus:
            path += iter->get<ESM::Apparatus>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Armor:
            path += iter->get<ESM::Armor>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Book:
            path += iter->get<ESM::Book>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Clothing:
            path += iter->get<ESM::Clothing>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Ingredient:
            path += iter->get<ESM::Ingredient>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Light:
            path += iter->get<ESM::Light>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Lockpick:
            path += iter->get<ESM::Tool>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Miscellaneous:
            path += iter->get<ESM::Miscellaneous>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Probe:
            path += iter->get<ESM::Probe>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Repair:
            path += iter->get<ESM::Repair>()->base->icon;
            break;
        case MWWorld::ContainerStore::Type_Weapon:
            path += iter->get<ESM::Weapon>()->base->icon;
            break;


        }
        count++;

        if(count % 8 == 0)
        {
            y += 36;
            x = 4;
            count = 0;
        }
        x += 36;


        MyGUI::ImageBox* image = containerWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(x, y, 32, 32), MyGUI::Align::Default);
        MyGUI::TextBox* text = containerWidget->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(x, y, 18, 18), MyGUI::Align::Default, std::string("Label"));

        if(iter->getRefData().getCount() > 1)
            text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()));


        containerWidgets.push_back(image);


        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");

        //std::cout << path << std::endl;
        image->setImageTexture(path);
    }


    setVisible(true);
}

void Update()
{

}

