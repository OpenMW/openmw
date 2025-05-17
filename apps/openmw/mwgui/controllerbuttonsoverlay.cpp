#include "controllerbuttonsoverlay.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

namespace MWGui
{
    ControllerButtonsOverlay::ControllerButtonsOverlay()
        : WindowBase("openmw_controllerbuttons.layout")
    {
        getWidget(mButtonStr, "ButtonStr");
    }

    void ControllerButtonsOverlay::setButtonStr(const std::string& buttonStr)
    {
        mButtonStr->setCaptionWithReplacing(buttonStr);

        // int height = mMessage->getTextSize().height + 60;
        // int width = mMessage->getTextSize().width + 24;
        // mMainWidget->setSize(width, height);
        // mMessage->setSize(mMessage->getWidth(), mMessage->getTextSize().height + 24);
    }
}
