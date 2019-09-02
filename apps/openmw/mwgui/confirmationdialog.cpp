#include "confirmationdialog.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

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

    void ConfirmationDialog::askForConfirmation(const std::string& message)
    {
        setVisible(true);

        mMessage->setCaptionWithReplacing(message);

        int height = mMessage->getTextSize().height + 60;

        int width = mMessage->getTextSize().width + 24;

        mMainWidget->setSize(width, height);

        mMessage->setSize(mMessage->getWidth(), mMessage->getTextSize().height + 24);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mOkButton);

        center();
    }

    bool ConfirmationDialog::exit()
    {
        setVisible(false);
        eventCancelClicked();
        return true;
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
