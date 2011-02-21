#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include <openengine/gui/layout.hpp>

namespace MWGui
{
    class WindowManager;

    class WindowBase: public OEngine::GUI::Layout
    {
        public:
        WindowBase(const std::string& parLayout, WindowManager& parWindowManager);

        // Events
        typedef MyGUI::delegates::CDelegate1<WindowBase*> EventHandle_WindowBase;

        virtual void open();
        void center();

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

        protected:
        WindowManager& mWindowManager;
    };
}

#endif

