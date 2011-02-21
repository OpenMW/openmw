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

        virtual void open();
        void center();

        protected:
        WindowManager& mWindowManager;
    };
}

#endif

