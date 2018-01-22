#ifndef MWGUI_MESSAGE_BOX_H
#define MWGUI_MESSAGE_BOX_H

#include "windowbase.hpp"

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
    class MessageBoxManager
    {
        public:
            MessageBoxManager (float timePerChar);
            ~MessageBoxManager ();
            void onFrame (float frameDuration);
            void createMessageBox (const std::string& message, bool stat = false);
            void removeStaticMessageBox ();
            bool createInteractiveMessageBox (const std::string& message, const std::vector<std::string>& buttons);
            bool isInteractiveMessageBox ();

            const InteractiveMessageBox* getInteractiveMessageBox() const { return mInterMessageBoxe; }

            /// Remove all message boxes
            void clear();

            bool removeMessageBox (MessageBox *msgbox);

            /// @param reset Reset the pressed button to -1 after reading it.
            int readPressedButton (bool reset=true);

            typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_Int;

            // Note: this delegate unassigns itself after it was fired, i.e. works once.
            EventHandle_Int eventButtonPressed;

            void onButtonPressed(int button) { eventButtonPressed(button); eventButtonPressed.clear(); }

        private:
            std::vector<MessageBox*> mMessageBoxes;
            InteractiveMessageBox* mInterMessageBoxe;
            MessageBox* mStaticMessageBox;
            float mMessageBoxSpeed;
            int mLastButtonPressed;
    };

    class MessageBox : public Layout
    {
        public:
            MessageBox (MessageBoxManager& parMessageBoxManager, const std::string& message);
            void setMessage (const std::string& message);
            int getHeight ();
            void update (int height);

            float mCurrentTime;
            float mMaxTime;

        protected:
            MessageBoxManager& mMessageBoxManager;
            const std::string& mMessage;
            MyGUI::EditBox* mMessageWidget;
            int mBottomPadding;
            int mNextBoxPadding;
    };

    class InteractiveMessageBox : public WindowModal
    {
        public:
            InteractiveMessageBox (MessageBoxManager& parMessageBoxManager, const std::string& message, const std::vector<std::string>& buttons);
            void mousePressed (MyGUI::Widget* _widget);
            int readPressedButton ();

            MyGUI::Widget* getDefaultKeyFocus() override;

            virtual bool exit() { return false; }

            bool mMarkedToDelete;

        private:
            void buttonActivated (MyGUI::Widget* _widget);

            MessageBoxManager& mMessageBoxManager;
            MyGUI::EditBox* mMessageWidget;
            MyGUI::Widget* mButtonsWidget;
            std::vector<MyGUI::Button*> mButtons;

            int mButtonPressed;
    };

}

#endif
