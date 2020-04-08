#ifndef MWINPUT_SDLMAPPINGS_H
#define MWINPUT_SDLMAPPINGS_H

#include <string>

#include <SDL_types.h>

namespace MyGUI
{
    struct MouseButton;
}

namespace MWInput
{
    std::string sdlControllerButtonToString(int button);

    std::string sdlControllerAxisToString(int axis);

    MyGUI::MouseButton sdlButtonToMyGUI(Uint8 button);
}
#endif
