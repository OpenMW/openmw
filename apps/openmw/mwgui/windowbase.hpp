#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include <openengine/gui/layout.hpp>

namespace MWBase
{
    class WindowManager;
}

namespace MWGui
{
    class WindowManager;
    class DragAndDrop;

    class WindowBase: public OEngine::GUI::Layout
    {
        public:
        WindowBase(const std::string& parLayout);

        // Events
        typedef MyGUI::delegates::CMultiDelegate1<WindowBase*> EventHandle_WindowBase;

        ///Unhides the window
        virtual void open() {}
        ///Hides the window
        virtual void close () {}
        ///Gracefully exits the window
        virtual void exit() {}
        ///Sets the visibility of the window
        virtual void setVisible(bool visible);
        ///Returns the visibility state of the window
        virtual bool isVisible();
        void center();
    };


    /*
     * "Modal" windows cause the rest of the interface to be unaccessible while they are visible
     */
    class WindowModal : public WindowBase
    {
    public:
        WindowModal(const std::string& parLayout);
        virtual void open();
        virtual void close();
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
