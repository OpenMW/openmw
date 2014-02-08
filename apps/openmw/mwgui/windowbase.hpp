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

        virtual void open() {}
        virtual void close () {}
        virtual void setVisible(bool visible);
        void center();

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;
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
    };

    /// A window that cannot be the target of a drag&drop action.
    /// When hovered with a drag item, the window will become transparent and allow click-through.
    class NoDrop
    {
    public:
        NoDrop(DragAndDrop* drag, MyGUI::Widget* widget);

        void onFrame(float dt);

    private:
        MyGUI::Widget* mWidget;
        DragAndDrop* mDrag;
        bool mTransparent;
    };
}

#endif
