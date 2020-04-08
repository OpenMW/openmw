#include "sdlmappings.hpp"

#include <MyGUI_MouseButton.h>

#include <SDL_gamecontroller.h>
#include <SDL_mouse.h>

namespace MWInput
{
    std::string sdlControllerButtonToString(int button)
    {
        switch(button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                return "A Button";
            case SDL_CONTROLLER_BUTTON_B:
                return "B Button";
            case SDL_CONTROLLER_BUTTON_BACK:
                return "Back Button";
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return "DPad Down";
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return "DPad Left";
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return "DPad Right";
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return "DPad Up";
            case SDL_CONTROLLER_BUTTON_GUIDE:
                return "Guide Button";
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                return "Left Shoulder";
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                return "Left Stick Button";
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                return "Right Shoulder";
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                return "Right Stick Button";
            case SDL_CONTROLLER_BUTTON_START:
                return "Start Button";
            case SDL_CONTROLLER_BUTTON_X:
                return "X Button";
            case SDL_CONTROLLER_BUTTON_Y:
                return "Y Button";
            default:
                return "Button " + std::to_string(button);
        }
    }

    std::string sdlControllerAxisToString(int axis)
    {
        switch(axis)
        {
            case SDL_CONTROLLER_AXIS_LEFTX:
                return "Left Stick X";
            case SDL_CONTROLLER_AXIS_LEFTY:
                return "Left Stick Y";
            case SDL_CONTROLLER_AXIS_RIGHTX:
                return "Right Stick X";
            case SDL_CONTROLLER_AXIS_RIGHTY:
                return "Right Stick Y";
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                return "Left Trigger";
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                return "Right Trigger";
            default:
                return "Axis " + std::to_string(axis);
        }
    }

    MyGUI::MouseButton sdlButtonToMyGUI(Uint8 button)
    {
        //The right button is the second button, according to MyGUI
        if(button == SDL_BUTTON_RIGHT)
            button = SDL_BUTTON_MIDDLE;
        else if(button == SDL_BUTTON_MIDDLE)
            button = SDL_BUTTON_RIGHT;

        //MyGUI's buttons are 0 indexed
        return MyGUI::MouseButton::Enum(button - 1);
    }
}
