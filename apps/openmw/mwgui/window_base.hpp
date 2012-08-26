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

    class WindowBase: public OEngine::GUI::Layout
    {
        public:
        WindowBase(const std::string& parLayout, MWBase::WindowManager& parWindowManager);

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

        protected:
        /// \todo remove
        MWBase::WindowManager& mWindowManager;
    };


    /*
     * "Modal" windows cause the rest of the interface to be unaccessible while they are visible
     */
    class WindowModal : public WindowBase
    {
    public:
        WindowModal(const std::string& parLayout, MWBase::WindowManager& parWindowManager);
        virtual void open();
        virtual void close();
    };
}

#endif
