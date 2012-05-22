#include "settingswindow.hpp"

#include "window_manager.hpp"

namespace MWGui
{
    SettingsWindow::SettingsWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_settings_window_layout.xml", parWindowManager)
    {
        getWidget(mOkButton, "OkButton");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);

        center();

        int okSize = mOkButton->getTextSize().width + 24;
        mOkButton->setCoord(mMainWidget->getWidth()-16-okSize, mOkButton->getTop(),
                            okSize, mOkButton->getHeight());
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.setGuiMode(GM_Game);
    }
}
