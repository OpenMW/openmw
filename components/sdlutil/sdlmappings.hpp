#ifndef SDLUTIL_SDLMAPPINGS
#define SDLUTIL_SDLMAPPINGS

#include <string>

#include <MyGUI_KeyCode.h>

#include <SDL_keycode.h>

namespace MyGUI
{
    struct MouseButton;
}

namespace SDLUtil
{
    std::string sdlControllerButtonToString(int button);

    std::string sdlControllerAxisToString(int axis);

    MyGUI::MouseButton sdlMouseButtonToMyGui(Uint8 button);
    Uint8 myGuiMouseButtonToSdl(MyGUI::MouseButton button);

    MyGUI::KeyCode sdlKeyToMyGUI(SDL_Keycode code);
    SDL_Keycode myGuiKeyToSdl(MyGUI::KeyCode button);
}
#endif // !SDLUTIL_SDLMAPPINGS
