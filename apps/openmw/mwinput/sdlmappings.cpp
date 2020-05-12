#include "sdlmappings.hpp"

#include <map>

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

    void initKeyMap(std::map<SDL_Keycode, MyGUI::KeyCode>& keyMap)
    {
        keyMap[SDLK_UNKNOWN] = MyGUI::KeyCode::None;
        keyMap[SDLK_ESCAPE] = MyGUI::KeyCode::Escape;
        keyMap[SDLK_1] = MyGUI::KeyCode::One;
        keyMap[SDLK_2] = MyGUI::KeyCode::Two;
        keyMap[SDLK_3] = MyGUI::KeyCode::Three;
        keyMap[SDLK_4] = MyGUI::KeyCode::Four;
        keyMap[SDLK_5] = MyGUI::KeyCode::Five;
        keyMap[SDLK_6] = MyGUI::KeyCode::Six;
        keyMap[SDLK_7] = MyGUI::KeyCode::Seven;
        keyMap[SDLK_8] = MyGUI::KeyCode::Eight;
        keyMap[SDLK_9] = MyGUI::KeyCode::Nine;
        keyMap[SDLK_0] = MyGUI::KeyCode::Zero;
        keyMap[SDLK_MINUS] = MyGUI::KeyCode::Minus;
        keyMap[SDLK_EQUALS] = MyGUI::KeyCode::Equals;
        keyMap[SDLK_BACKSPACE] = MyGUI::KeyCode::Backspace;
        keyMap[SDLK_TAB] = MyGUI::KeyCode::Tab;
        keyMap[SDLK_q] = MyGUI::KeyCode::Q;
        keyMap[SDLK_w] = MyGUI::KeyCode::W;
        keyMap[SDLK_e] = MyGUI::KeyCode::E;
        keyMap[SDLK_r] = MyGUI::KeyCode::R;
        keyMap[SDLK_t] = MyGUI::KeyCode::T;
        keyMap[SDLK_y] = MyGUI::KeyCode::Y;
        keyMap[SDLK_u] = MyGUI::KeyCode::U;
        keyMap[SDLK_i] = MyGUI::KeyCode::I;
        keyMap[SDLK_o] = MyGUI::KeyCode::O;
        keyMap[SDLK_p] = MyGUI::KeyCode::P;
        keyMap[SDLK_RETURN] = MyGUI::KeyCode::Return;
        keyMap[SDLK_a] = MyGUI::KeyCode::A;
        keyMap[SDLK_s] = MyGUI::KeyCode::S;
        keyMap[SDLK_d] = MyGUI::KeyCode::D;
        keyMap[SDLK_f] = MyGUI::KeyCode::F;
        keyMap[SDLK_g] = MyGUI::KeyCode::G;
        keyMap[SDLK_h] = MyGUI::KeyCode::H;
        keyMap[SDLK_j] = MyGUI::KeyCode::J;
        keyMap[SDLK_k] = MyGUI::KeyCode::K;
        keyMap[SDLK_l] = MyGUI::KeyCode::L;
        keyMap[SDLK_SEMICOLON] = MyGUI::KeyCode::Semicolon;
        keyMap[SDLK_QUOTE] = MyGUI::KeyCode::Apostrophe;
        keyMap[SDLK_BACKQUOTE] = MyGUI::KeyCode::Grave;
        keyMap[SDLK_LSHIFT] = MyGUI::KeyCode::LeftShift;
        keyMap[SDLK_BACKSLASH] = MyGUI::KeyCode::Backslash;
        keyMap[SDLK_z] = MyGUI::KeyCode::Z;
        keyMap[SDLK_x] = MyGUI::KeyCode::X;
        keyMap[SDLK_c] = MyGUI::KeyCode::C;
        keyMap[SDLK_v] = MyGUI::KeyCode::V;
        keyMap[SDLK_b] = MyGUI::KeyCode::B;
        keyMap[SDLK_n] = MyGUI::KeyCode::N;
        keyMap[SDLK_m] = MyGUI::KeyCode::M;
        keyMap[SDLK_COMMA] = MyGUI::KeyCode::Comma;
        keyMap[SDLK_PERIOD] = MyGUI::KeyCode::Period;
        keyMap[SDLK_SLASH] = MyGUI::KeyCode::Slash;
        keyMap[SDLK_RSHIFT] = MyGUI::KeyCode::RightShift;
        keyMap[SDLK_KP_MULTIPLY] = MyGUI::KeyCode::Multiply;
        keyMap[SDLK_LALT] = MyGUI::KeyCode::LeftAlt;
        keyMap[SDLK_SPACE] = MyGUI::KeyCode::Space;
        keyMap[SDLK_CAPSLOCK] = MyGUI::KeyCode::Capital;
        keyMap[SDLK_F1] = MyGUI::KeyCode::F1;
        keyMap[SDLK_F2] = MyGUI::KeyCode::F2;
        keyMap[SDLK_F3] = MyGUI::KeyCode::F3;
        keyMap[SDLK_F4] = MyGUI::KeyCode::F4;
        keyMap[SDLK_F5] = MyGUI::KeyCode::F5;
        keyMap[SDLK_F6] = MyGUI::KeyCode::F6;
        keyMap[SDLK_F7] = MyGUI::KeyCode::F7;
        keyMap[SDLK_F8] = MyGUI::KeyCode::F8;
        keyMap[SDLK_F9] = MyGUI::KeyCode::F9;
        keyMap[SDLK_F10] = MyGUI::KeyCode::F10;
        keyMap[SDLK_NUMLOCKCLEAR] = MyGUI::KeyCode::NumLock;
        keyMap[SDLK_SCROLLLOCK] = MyGUI::KeyCode::ScrollLock;
        keyMap[SDLK_KP_7] = MyGUI::KeyCode::Numpad7;
        keyMap[SDLK_KP_8] = MyGUI::KeyCode::Numpad8;
        keyMap[SDLK_KP_9] = MyGUI::KeyCode::Numpad9;
        keyMap[SDLK_KP_MINUS] = MyGUI::KeyCode::Subtract;
        keyMap[SDLK_KP_4] = MyGUI::KeyCode::Numpad4;
        keyMap[SDLK_KP_5] = MyGUI::KeyCode::Numpad5;
        keyMap[SDLK_KP_6] = MyGUI::KeyCode::Numpad6;
        keyMap[SDLK_KP_PLUS] = MyGUI::KeyCode::Add;
        keyMap[SDLK_KP_1] = MyGUI::KeyCode::Numpad1;
        keyMap[SDLK_KP_2] = MyGUI::KeyCode::Numpad2;
        keyMap[SDLK_KP_3] = MyGUI::KeyCode::Numpad3;
        keyMap[SDLK_KP_0] = MyGUI::KeyCode::Numpad0;
        keyMap[SDLK_KP_PERIOD] = MyGUI::KeyCode::Decimal;
        keyMap[SDLK_F11] = MyGUI::KeyCode::F11;
        keyMap[SDLK_F12] = MyGUI::KeyCode::F12;
        keyMap[SDLK_F13] = MyGUI::KeyCode::F13;
        keyMap[SDLK_F14] = MyGUI::KeyCode::F14;
        keyMap[SDLK_F15] = MyGUI::KeyCode::F15;
        keyMap[SDLK_KP_EQUALS] = MyGUI::KeyCode::NumpadEquals;
        keyMap[SDLK_COLON] = MyGUI::KeyCode::Colon;
        keyMap[SDLK_KP_ENTER] = MyGUI::KeyCode::NumpadEnter;
        keyMap[SDLK_KP_DIVIDE] = MyGUI::KeyCode::Divide;
        keyMap[SDLK_SYSREQ] = MyGUI::KeyCode::SysRq;
        keyMap[SDLK_RALT] = MyGUI::KeyCode::RightAlt;
        keyMap[SDLK_HOME] = MyGUI::KeyCode::Home;
        keyMap[SDLK_UP] = MyGUI::KeyCode::ArrowUp;
        keyMap[SDLK_PAGEUP] = MyGUI::KeyCode::PageUp;
        keyMap[SDLK_LEFT] = MyGUI::KeyCode::ArrowLeft;
        keyMap[SDLK_RIGHT] = MyGUI::KeyCode::ArrowRight;
        keyMap[SDLK_END] = MyGUI::KeyCode::End;
        keyMap[SDLK_DOWN] = MyGUI::KeyCode::ArrowDown;
        keyMap[SDLK_PAGEDOWN] = MyGUI::KeyCode::PageDown;
        keyMap[SDLK_INSERT] = MyGUI::KeyCode::Insert;
        keyMap[SDLK_DELETE] = MyGUI::KeyCode::Delete;
        keyMap[SDLK_APPLICATION] = MyGUI::KeyCode::AppMenu;

//The function of the Ctrl and Meta keys are switched on macOS compared to other platforms.
//For instance] = Cmd+C versus Ctrl+C to copy from the system clipboard
#if defined(__APPLE__)
        keyMap[SDLK_LGUI] = MyGUI::KeyCode::LeftControl;
        keyMap[SDLK_RGUI] = MyGUI::KeyCode::RightControl;
        keyMap[SDLK_LCTRL] = MyGUI::KeyCode::LeftWindows;
        keyMap[SDLK_RCTRL] = MyGUI::KeyCode::RightWindows;
#else
        keyMap[SDLK_LGUI] = MyGUI::KeyCode::LeftWindows;
        keyMap[SDLK_RGUI] = MyGUI::KeyCode::RightWindows;
        keyMap[SDLK_LCTRL] = MyGUI::KeyCode::LeftControl;
        keyMap[SDLK_RCTRL] = MyGUI::KeyCode::RightControl;
#endif
    }

    MyGUI::KeyCode sdlKeyToMyGUI(SDL_Keycode code)
    {
        static std::map<SDL_Keycode, MyGUI::KeyCode> keyMap;
        if (keyMap.empty())
            initKeyMap(keyMap);

        MyGUI::KeyCode kc = MyGUI::KeyCode::None;
        auto foundKey = keyMap.find(code);
        if (foundKey != keyMap.end())
            kc = foundKey->second;

        return kc;
    }
}
