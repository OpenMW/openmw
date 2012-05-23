#ifndef MWGUI_CONFIRMATIONDIALOG_H
#define MWGUI_CONFIRMATIONDIALOG_H

#include "window_base.hpp"

namespace MWGui
{
    class ConfirmationDialog : public WindowBase
    {
        public:
            ConfirmationDialog(WindowManager& parWindowManager);
            void open(const std::string& message);

            typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

            /** Event : Ok button was clicked.\n
                signature : void method()\n
            */
            EventHandle_Void eventOkClicked;

        private:
            MyGUI::EditBox* mMessage;
            MyGUI::Button* mOkButton;
            MyGUI::Button* mCancelButton;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onOkButtonClicked(MyGUI::Widget* _sender);

            void close();
    };

}

#endif
