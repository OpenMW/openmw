#include "controllers.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_Widget.h>

namespace MWGui
{
    namespace Controllers
    {

        ControllerRepeatEvent::ControllerRepeatEvent() :
            mInit(0.5f),
            mStep(0.1f),
            mEnabled(true),
            mTimeLeft(0)
        {
        }

        ControllerRepeatEvent::~ControllerRepeatEvent()
        {
        }

        bool ControllerRepeatEvent::addTime(MyGUI::Widget* _widget, float _time)
        {
            if(mTimeLeft == 0)
                mTimeLeft = mInit;

            mTimeLeft -= _time;
            while (mTimeLeft <= 0)
            {
                mTimeLeft += mStep;
                eventRepeatClick(_widget, this);
            }
            return true;
        }

        void ControllerRepeatEvent::setRepeat(float init, float step)
        {
            mInit = init;
            mStep = step;
        }

        void ControllerRepeatEvent::setEnabled(bool enable)
        {
            mEnabled = enable;
        }

        void ControllerRepeatEvent::setProperty(const std::string& _key, const std::string& _value)
        {
        }

        void ControllerRepeatEvent::prepareItem(MyGUI::Widget* _widget)
        {
        }

        // -------------------------------------------------------------

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
