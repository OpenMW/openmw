#ifndef MWINPUT_SDLMAPPINGS_H
#define MWINPUT_SDLMAPPINGS_H

#include <string>

#include <MyGUI_KeyCode.h>

#include <SDL_keycode.h>

namespace MyGUI
{
    struct MouseButton;
}

namespace MWInput
{
    std::string sdlControllerButtonToString(int button);

    std::string sdlControllerAxisToString(int axis);

    MyGUI::MouseButton sdlButtonToMyGUI(Uint8 button);

    MyGUI::KeyCode sdlKeyToMyGUI(SDL_Keycode code);
}
#endif
