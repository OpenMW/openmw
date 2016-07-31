#include "confirmationdialog.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

namespace MWGui
{
    ConfirmationDialog::ConfirmationDialog() :
        WindowModal("openmw_confirmation_dialog.layout")
    {
        getWidget(mMessage, "Message");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfirmationDialog::onCancelButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfirmationDialog::onOkButtonClicked);
    }

    void ConfirmationDialog::askForConfirmation(const std::string& message, const std::string& confirmMessage, const std::string& cancelMessage)
    {
        setVisible(true);

        mMessage->setCaptionWithReplacing(message);

        mCancelButton->setCaptionWithReplacing(cancelMessage);
        mOkButton->setCaptionWithReplacing(confirmMessage);

        int height = mMessage->getTextSize().height + 72;

        mMainWidget->setSize(mMainWidget->getWidth(), height);

        mMessage->setSize(mMessage->getWidth(), mMessage->getTextSize().height+24);

        center();
    }

    void ConfirmationDialog::exit()
    {
        setVisible(false);

        eventCancelClicked();
    }

    void ConfirmationDialog::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void ConfirmationDialog::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        setVisible(false);

        eventOkClicked();
    }
}
