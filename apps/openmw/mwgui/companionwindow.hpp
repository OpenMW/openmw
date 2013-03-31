#ifndef OPENMW_MWGUI_COMPANIONWINDOW_H
#define OPENMW_MWGUI_COMPANIONWINDOW_H

#include "container.hpp"
#include "widgets.hpp"

namespace MWGui
{
    class MessageBoxManager;

    class CompanionWindow : public ContainerBase, public WindowBase
    {
    public:
        CompanionWindow(MWBase::WindowManager& parWindowManager,DragAndDrop* dragAndDrop, MessageBoxManager* manager);
        virtual ~CompanionWindow() {}

        void open(MWWorld::Ptr npc);

        virtual void notifyItemDragged(MWWorld::Ptr item, int count);

    protected:
        MyGUI::Button* mCloseButton;
        MyGUI::TextBox* mProfitLabel;
        Widgets::MWDynamicStat* mEncumbranceBar;
        MessageBoxManager* mMessageBoxManager;

        void onMessageBoxButtonClicked(int button);

        void updateEncumbranceBar();

        void onWindowResize(MyGUI::Window* window);
        void onCloseButtonClicked(MyGUI::Widget* _sender);

        virtual void onReferenceUnavailable();
    };

}

#endif
