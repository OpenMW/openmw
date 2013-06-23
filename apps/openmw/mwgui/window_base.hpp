#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include <openengine/gui/layout.hpp>

namespace MWWorld
{
    class Environment;
}

namespace MWGui
{
    class WindowBase: public OEngine::GUI::Layout
    {
        public:
        WindowBase(const std::string& parLayout, MWWorld::Environment& parEnvironment);

        virtual void open();
        void center();

        protected:
        MWWorld::Environment& environment;
    };
}

#endif

