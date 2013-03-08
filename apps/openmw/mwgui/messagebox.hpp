#ifndef MWGUI_MESSAGE_BOX_H
#define MWGUI_MESSAGE_BOX_H

#include <openengine/gui/layout.hpp>

#include "window_base.hpp"

#include "../mwbase/windowmanager.hpp"

#undef MessageBox

namespace MyGUI
{
    class Widget;
    class Button;
    class EditBox;
}

namespace MWGui
{
    class InteractiveMessageBox;
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
            MessageBoxManager (MWBase::WindowManager* windowManager);
            void onFrame (float frameDuration);
            void createMessageBox (const std::string& message);
            bool createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
            bool isInteractiveMessageBox ();

            void removeMessageBox (float time, MessageBox *msgbox);
            bool removeMessageBox (MessageBox *msgbox);
            void setMessageBoxSpeed (int speed);
            
            void enterPressed();
            int readPressedButton ();

            MWBase::WindowManager *mWindowManager;

        private:
            std::vector<MessageBox*> mMessageBoxes;
            InteractiveMessageBox* mInterMessageBoxe;
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
            const std::string& mMessage;
            MyGUI::EditBox* mMessageWidget;
            int mFixedWidth;
            int mBottomPadding;
            int mNextBoxPadding;
    };

    class InteractiveMessageBox : public OEngine::GUI::Layout
    {
        public:
            InteractiveMessageBox (MessageBoxManager& parMessageBoxManager, const std::string& message, const std::vector<std::string>& buttons);
            void enterPressed ();
            void mousePressed (MyGUI::Widget* _widget);
            int readPressedButton ();

            bool mMarkedToDelete;

        private:
            void buttonActivated (MyGUI::Widget* _widget);
            
            MessageBoxManager& mMessageBoxManager;
            MyGUI::EditBox* mMessageWidget;
            MyGUI::Widget* mButtonsWidget;
            std::vector<MyGUI::Button*> mButtons;

            int mTextButtonPadding;
            int mButtonPressed;
    };

}

#endif
