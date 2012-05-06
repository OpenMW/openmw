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
#include <vector>

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
    class ContainerWindow;
}


namespace MWGui
{
    class DragAndDrop
    {
    public:
        bool mIsOnDragAndDrop;
        ContainerWindow* mContainerWindow;
        MyGUI::Widget* mDraggedWidget;
        MyGUI::Widget* mDragAndDropWidget;

        MWWorld::Ptr mItem;
    };

    class ContainerWindow : public WindowBase
    {
    public:
        ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,DragAndDrop* dragAndDrop);
        ContainerWindow(WindowManager& parWindowManager,MWWorld::Environment& environment,DragAndDrop* dragAndDrop,
            std::string guiFile);


        void open(MWWorld::Ptr& container);
        void setName(std::string contName);
        void Update();

        virtual ~ContainerWindow();

    protected:
        MWWorld::Environment& mEnvironment;
        std::vector<MyGUI::WidgetPtr> mContainerWidgets;
        MyGUI::ItemBoxPtr mContainerWidget;

        MyGUI::ButtonPtr takeButton;
        MyGUI::ButtonPtr closeButton;
        DragAndDrop* mDragAndDrop;

        MWWorld::Ptr mContainer;
        bool mIsValid;//is in the right GUI Mode

        void drawItems();

        void onByeClicked(MyGUI::Widget* _sender);
        void onSelectedItem(MyGUI::Widget* _sender);
        void onContainerClicked(MyGUI::Widget* _sender);
        void onMouseMove(MyGUI::Widget* _sender, int _left, int _top);

        //MWWorld::Ptr& mContainer;
    };
}
#endif // CONTAINER_H
