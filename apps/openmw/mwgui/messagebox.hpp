#ifndef MWGUI_MESSAGE_BOX_H
#define MWGUI_MESSAGE_BOX_H

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    using namespace MyGUI;

    class WindowManager;

    class MessageBoxManager
    {
        public:
            void createMessageBox (const std::string& message);
            void createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
    };
}

#endif
