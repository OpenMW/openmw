#include "controllers.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

namespace MWGui
{
    namespace Controllers
    {
        void ControllerFollowMouse::prepareItem(MyGUI::Widget *_widget)
        {
        }

        bool ControllerFollowMouse::addTime(MyGUI::Widget *_widget, float _time)
        {
            _widget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
            return true;
        }

    }
}
