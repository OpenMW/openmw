#ifndef MWGUI_MESSAGE_BOX_H
#define MWGUI_MESSAGE_BOX_H

#include <openengine/gui/layout.hpp>
#include <MyGUI.h>

#include "window_base.hpp"
#include "window_manager.hpp"

namespace MWGui
{

    class MessageBoxManager;
    class MessageBox;

    class MessageBoxManager
    {
        public:
            MessageBoxManager (WindowManager* windowManager);
            void createMessageBox (const std::string& message);
            void createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
            
            WindowManager *mWindowManager;
            
        private:
            std::vector<MessageBox*> mMessageBoxes;
    };
    
    class MessageBox : public OEngine::GUI::Layout
    {
        public:
            MessageBox (MessageBoxManager& parMessageBoxManager, const std::string& message);
            void setMessage (const std::string& message);
            int getHeight ();
            void update (int height);
            
        protected:
            MessageBoxManager& mMessageBoxManager;
            int mHeight;
            const std::string& cMessage;
            MyGUI::EditPtr mMessageWidget;
            int mFixedWidth;
            int mBottomPadding;
            int mNextBoxPadding;
    };

}

#endif
