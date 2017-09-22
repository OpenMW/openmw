#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include "layout.hpp"

namespace MWBase
{
    class WindowManager;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    class WindowManager;
    class DragAndDrop;

    class WindowBase: public Layout
    {
        public:
        WindowBase(const std::string& parLayout);

        // Events
        typedef MyGUI::delegates::CMultiDelegate1<WindowBase*> EventHandle_WindowBase;

        /// Open this object in the GUI, for windows that support it
        virtual void setPtr(const MWWorld::Ptr& ptr) {}

        /// Notify that window has been made visible
        virtual void onOpen() {}
        /// Notify that window has been hidden
        virtual void onClose () {}
        /// Gracefully exits the window
        virtual void exit() {}
        /// Sets the visibility of the window
        virtual void setVisible(bool visible);
        /// Returns the visibility state of the window
        bool isVisible();
        void center();
    };


    /*
     * "Modal" windows cause the rest of the interface to be unaccessible while they are visible
     */
    class WindowModal : public WindowBase
    {
    public:
        WindowModal(const std::string& parLayout);
        virtual void onOpen();
        virtual void onClose();
        virtual void exit() {}
    };

    /// A window that cannot be the target of a drag&drop action.
    /// When hovered with a drag item, the window will become transparent and allow click-through.
    class NoDrop
    {
    public:
        NoDrop(DragAndDrop* drag, MyGUI::Widget* widget);

        void onFrame(float dt);
        virtual void setAlpha(float alpha);

    private:
        MyGUI::Widget* mWidget;
        DragAndDrop* mDrag;
        bool mTransparent;
    };
}

#endif
