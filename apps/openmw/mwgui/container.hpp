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
        MyGUI::Widget* mDraggedWidget;
        MyGUI::Widget* mDragAndDropWidget;
        MWWorld::ContainerStore mStore;
    };

    class ContainerBase
    {
    public:
        ContainerBase(DragAndDrop* dragAndDrop);
        virtual ~ContainerBase();

        enum Filter
        {
            Filter_All = 0x01,
            Filter_Weapon = 0x02,
            Filter_Apparel = 0x03,
            Filter_Magic = 0x04,
            Filter_Misc = 0x05
        };

        void setWidgets(MyGUI::Widget* containerWidget, MyGUI::ScrollView* itemView); ///< only call once

        void openContainer(MWWorld::Ptr container);
        void setFilter(Filter filter); ///< set category filter
        void Update();

    protected:
        MyGUI::ScrollView* mItemView;
        MyGUI::Widget* mContainerWidget;

        MyGUI::Widget* mSelectedItem;

        DragAndDrop* mDragAndDrop;
        MWWorld::Ptr mContainer;

        Filter mFilter;

        void onSelectedItem(MyGUI::Widget* _sender);
        void onSelectedItemImpl(MyGUI::Widget* _sender, int count);
        void onContainerClicked(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);

        std::string getCountString(const int count);

        void drawItems();
    };

    class ContainerWindow : public ContainerBase, public WindowBase
    {
    public:
        ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop);

        virtual ~ContainerWindow();

        void open(MWWorld::Ptr container);

    protected:
        MyGUI::Button* mTakeButton;
        MyGUI::Button* mCloseButton;

        void onWindowResize(MyGUI::Window* window);
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeAllButtonClicked(MyGUI::Widget* _sender);
    };
}
#endif // CONTAINER_H
