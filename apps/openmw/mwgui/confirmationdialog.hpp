#ifndef MWGUI_CONFIRMATIONDIALOG_H
#define MWGUI_CONFIRMATIONDIALOG_H

#include "windowbase.hpp"

namespace MWGui
{
    class ConfirmationDialog : public WindowModal
    {
        public:
            ConfirmationDialog();
            void open(const std::string& message, const std::string& confirmMessage="#{sOk}", const std::string& cancelMessage="#{sCancel}");
            virtual void exit();

            typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

            /** Event : Ok button was clicked.\n
                signature : void method()\n
            */
            EventHandle_Void eventOkClicked;
            EventHandle_Void eventCancelClicked;

        private:
            MyGUI::EditBox* mMessage;
            MyGUI::Button* mOkButton;
            MyGUI::Button* mCancelButton;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onOkButtonClicked(MyGUI::Widget* _sender);
    };

}

#endif
