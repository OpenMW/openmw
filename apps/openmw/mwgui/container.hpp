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
    class ContainerBase;
}


namespace MWGui
{
    class DragAndDrop
    {
    public:
        bool mIsOnDragAndDrop;
        ContainerBase* mContainerWindow;
        MyGUI::Widget* mDraggedWidget;
        MyGUI::Widget* mDragAndDropWidget;
        MWWorld::ContainerStore mStore;
        MWWorld::Ptr mItem;
    };

    class ContainerBase : public WindowBase
    {
    public:
        ContainerBase(WindowManager& parWindowManager, DragAndDrop* dragAndDrop, std::string guiFile);
        virtual ~ContainerBase();

        enum Filter
        {
            Filter_All = 0x01,
            Filter_Weapon = 0x02,
            Filter_Apparel = 0x03,
            Filter_Magic = 0x04,
            Filter_Misc = 0x05
        };

        void open(MWWorld::Ptr container);
        void setName(std::string contName);
        void setFilter(Filter filter); ///< set category filter
        void Update();

    protected:
        MyGUI::ScrollView* mItemView;
        MyGUI::Widget* mContainerWidget;

        DragAndDrop* mDragAndDrop;
        MWWorld::Ptr mContainer;

        Filter mFilter;

        void onSelectedItem(MyGUI::Widget* _sender);
        void onContainerClicked(MyGUI::Widget* _sender);

        void drawItems();
    };

    class ContainerWindow : public ContainerBase
    {
    public:
        ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

        virtual ~ContainerWindow();

    protected:
        std::vector<MyGUI::WidgetPtr> mContainerWidgets;

        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        bool mIsValid;//is in the right GUI Mode

        void onWindowResize(MyGUI::Window* window);
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeAllButtonClicked(MyGUI::Widget* _sender);
    };
}
#endif // CONTAINER_H
