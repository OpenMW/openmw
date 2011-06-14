#ifndef MWGUI_MESSAGE_BOX_H
#define MWGUI_MESSAGE_BOX_H

#include <openengine/gui/layout.hpp>
#include <MyGUI.h>

#include "window_base.hpp"

namespace MWGui
{

    class MessageBoxManager;
    class MessageBox;

    class MessageBoxManager
    {
        private:
            std::vector<MessageBox*> mMessageBoxes;
        public:
            void createMessageBox (const std::string& message);
            void createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
    };
    
    class MessageBox : public OEngine::GUI::Layout
    {
        public:
            MessageBox(MessageBoxManager& parMessageBoxManager, const std::string& message);
            void setMessage(const std::string& message);
        protected:
            MessageBoxManager& mMessageBoxManager;
    };

}

#endif
