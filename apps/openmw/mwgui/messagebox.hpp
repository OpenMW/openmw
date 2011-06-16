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

    struct MessageBoxManagerTimer {
        float current;
        float max;
        MessageBox *messageBox;
    };

    class MessageBoxManager
    {
        public:
            MessageBoxManager (WindowManager* windowManager);
            void onFrame (float frameDuration);
            void createMessageBox (const std::string& message);
            void createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
            
            void removeMessageBox (float time, MessageBox *msgbox);
            bool removeMessageBox (MessageBox *msgbox);
            void setMessageBoxSpeed (int speed);
            
            WindowManager *mWindowManager;
            
        private:
            std::vector<MessageBox*> mMessageBoxes;
            std::vector<MessageBoxManagerTimer> mTimers;
            float mMessageBoxSpeed;
    };
    
    class MessageBox : public OEngine::GUI::Layout
    {
        public:
            MessageBox (MessageBoxManager& parMessageBoxManager, const std::string& message);
            void setMessage (const std::string& message);
            int getHeight ();
            void update (int height);
            
            bool mMarkedToDelete;
            
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
