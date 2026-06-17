#include "controllers.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

namespace MWGui
{
    namespace Controllers
    {
        void ControllerFollowMouse::prepareItem(MyGUI::Widget* widget) {}

        bool ControllerFollowMouse::addTime(MyGUI::Widget* widget, float time)
        {
            widget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
            return true;
        }

    }
}
