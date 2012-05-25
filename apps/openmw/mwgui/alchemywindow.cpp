#include "alchemywindow.hpp"

#include "window_manager.hpp"

namespace MWGui
{
    AlchemyWindow::AlchemyWindow(WindowManager& parWindowManager)
        : WindowBase("openmw_alchemy_window_layout.xml", parWindowManager)
    {
        getWidget(mCreateButton, "CreateButton");
        getWidget(mCancelButton, "CancelButton");


        MyGUI::Widget* buttonBox = mCancelButton->getParent();
        int cancelButtonWidth = mCancelButton->getTextSize().width + 24;
        mCancelButton->setCoord(buttonBox->getWidth() - cancelButtonWidth,
                                mCancelButton->getTop(), cancelButtonWidth, mCancelButton->getHeight());
        int createButtonWidth = mCreateButton->getTextSize().width + 24;
        mCreateButton->setCoord(buttonBox->getWidth() - createButtonWidth - cancelButtonWidth - 4,
                                mCreateButton->getTop(), createButtonWidth, mCreateButton->getHeight());

        mCreateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCreateButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCancelButtonClicked);

        center();
    }

    void AlchemyWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.popGuiMode();
        mWindowManager.popGuiMode();
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
    }
}
