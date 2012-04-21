#ifndef MGUI_CONTAINER_H
#define MGUI_CONTAINER_H

#include <components/esm_store/store.hpp>
#include "../mwclass/container.hpp"
#include <sstream>
#include <set>
#include <string>
#include <utility>
#include "window_base.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/containerstore.hpp"

namespace MWWorld
{
    class Environment;
}

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{


    class ContainerWindow : public WindowBase
    {
        public:
            ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment);
            ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,std::string guiFile);


            void open(MWWorld::Ptr& container);
            void setName(std::string contName);
            void Update();

            virtual ~ContainerWindow();

        protected:
        MWWorld::Environment& mEnvironment;
        std::vector<MyGUI::WidgetPtr> containerWidgets;
        MyGUI::WidgetPtr containerWidget;

        MyGUI::ButtonPtr takeButton;
        MyGUI::ButtonPtr closeButton;


        void onByeClicked(MyGUI::Widget* _sender);


        //MWWorld::Ptr& mContainer;
    };
}
#endif // CONTAINER_H
