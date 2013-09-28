#include "confirmationdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

namespace MWGui
{
    ConfirmationDialog::ConfirmationDialog(MWBase::WindowManager& parWindowManager) :
        WindowModal("openmw_confirmation_dialog.layout", parWindowManager)
    {
        getWidget(mMessage, "Message");
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfirmationDialog::onCancelButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfirmationDialog::onOkButtonClicked);
    }

    void ConfirmationDialog::open(const std::string& message)
    {
        setVisible(true);

        mMessage->setCaptionWithReplacing(message);

        int height = mMessage->getTextSize().height + 72;

        mMainWidget->setSize(mMainWidget->getWidth(), height);

        mMessage->setSize(mMessage->getWidth(), mMessage->getTextSize().height+24);

        center();

        int okButtonWidth = mOkButton->getTextSize().width + 24;
        mOkButton->setCoord(mMainWidget->getWidth() - 30 - okButtonWidth,
                            mOkButton->getTop(),
                            okButtonWidth,
                            mOkButton->getHeight());

        int cancelButtonWidth = mCancelButton->getTextSize().width + 24;
        mCancelButton->setCoord(mMainWidget->getWidth() - 30 - okButtonWidth - cancelButtonWidth - 8,
                            mCancelButton->getTop(),
                            cancelButtonWidth,
                            mCancelButton->getHeight());
    }

    void ConfirmationDialog::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        eventCancelClicked();

        setVisible(false);
    }

    void ConfirmationDialog::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        eventOkClicked();

        setVisible(false);
    }
}
