#include "luabindings.hpp"

#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_mouse.h>

#include "../mwbase/inputmanager.hpp"
#include "../mwinput/actions.hpp"

namespace sol
{
    template <>
    struct is_automagical<SDL_Keysym> : std::false_type {};
}

namespace MWLua
{

    sol::table initInputPackage(const Context& context)
    {
        sol::usertype<SDL_Keysym> keyEvent = context.mLua->sol().new_usertype<SDL_Keysym>("KeyEvent");
        keyEvent["symbol"] = sol::readonly_property([](const SDL_Keysym& e)
        {
            if (e.sym > 0 && e.sym <= 255)
                return std::string(1, static_cast<char>(e.sym));
            else
                return std::string();
        });
        keyEvent["code"] = sol::readonly_property([](const SDL_Keysym& e) -> int { return e.scancode; });
        keyEvent["withShift"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_SHIFT; });
        keyEvent["withCtrl"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_CTRL; });
        keyEvent["withAlt"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_ALT; });
        keyEvent["withSuper"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_GUI; });

        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        sol::table api(context.mLua->sol(), sol::create);

        api["isIdle"] = [input]() { return input->isIdle(); };
        api["isActionPressed"] = [input](int action) { return input->actionIsActive(action); };
        api["isKeyPressed"] = [input](SDL_Scancode code) -> bool
        {
            int maxCode;
            const auto* state = SDL_GetKeyboardState(&maxCode);
            if (code >= 0 && code < maxCode)
                return state[code] != 0;
            else
                return false;
        };
        api["isShiftPressed"] = [input]() -> bool { return SDL_GetModState() & KMOD_SHIFT; };
        api["isCtrlPressed"] = [input]() -> bool { return SDL_GetModState() & KMOD_CTRL; };
        api["isAltPressed"] = [input]() -> bool { return SDL_GetModState() & KMOD_ALT; };
        api["isSuperPressed"] = [input]() -> bool { return SDL_GetModState() & KMOD_GUI; };
        api["isControllerButtonPressed"] = [input](int button)
        {
            return input->isControllerButtonPressed(static_cast<SDL_GameControllerButton>(button));
        };
        api["isMouseButtonPressed"] = [input](int button) -> bool
        {
            return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(button);
        };
        api["getMouseMoveX"] = [input]() { return input->getMouseMoveX(); };
        api["getMouseMoveY"] = [input]() { return input->getMouseMoveY(); };
        api["getAxisValue"] = [input](int axis)
        {
            if (axis < SDL_CONTROLLER_AXIS_MAX)
                return input->getControllerAxisValue(static_cast<SDL_GameControllerAxis>(axis));
            else
                return input->getActionValue(axis - SDL_CONTROLLER_AXIS_MAX) * 2 - 1;
        };

        api["getControlSwitch"] = [input](std::string_view key) { return input->getControlSwitch(key); };
        api["setControlSwitch"] = [input](std::string_view key, bool v) { input->toggleControlSwitch(key, v); };

        api["ACTION"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, MWInput::Actions>({
            {"GameMenu", MWInput::A_GameMenu},
            {"Screenshot", MWInput::A_Screenshot},
            {"Inventory", MWInput::A_Inventory},
            {"Console", MWInput::A_Console},

            {"MoveLeft", MWInput::A_MoveLeft},
            {"MoveRight", MWInput::A_MoveRight},
            {"MoveForward", MWInput::A_MoveForward},
            {"MoveBackward", MWInput::A_MoveBackward},

            {"Activate", MWInput::A_Activate},
            {"Use", MWInput::A_Use},
            {"Jump", MWInput::A_Jump},
            {"AutoMove", MWInput::A_AutoMove},
            {"Rest", MWInput::A_Rest},
            {"Journal", MWInput::A_Journal},
            {"Weapon", MWInput::A_Weapon},
            {"Spell", MWInput::A_Spell},
            {"Run", MWInput::A_Run},
            {"CycleSpellLeft", MWInput::A_CycleSpellLeft},
            {"CycleSpellRight", MWInput::A_CycleSpellRight},
            {"CycleWeaponLeft", MWInput::A_CycleWeaponLeft},
            {"CycleWeaponRight", MWInput::A_CycleWeaponRight},
            {"ToggleSneak", MWInput::A_ToggleSneak},
            {"AlwaysRun", MWInput::A_AlwaysRun},
            {"Sneak", MWInput::A_Sneak},

            {"QuickSave", MWInput::A_QuickSave},
            {"QuickLoad", MWInput::A_QuickLoad},
            {"QuickMenu", MWInput::A_QuickMenu},
            {"ToggleWeapon", MWInput::A_ToggleWeapon},
            {"ToggleSpell", MWInput::A_ToggleSpell},
            {"TogglePOV", MWInput::A_TogglePOV},

            {"QuickKey1", MWInput::A_QuickKey1},
            {"QuickKey2", MWInput::A_QuickKey2},
            {"QuickKey3", MWInput::A_QuickKey3},
            {"QuickKey4", MWInput::A_QuickKey4},
            {"QuickKey5", MWInput::A_QuickKey5},
            {"QuickKey6", MWInput::A_QuickKey6},
            {"QuickKey7", MWInput::A_QuickKey7},
            {"QuickKey8", MWInput::A_QuickKey8},
            {"QuickKey9", MWInput::A_QuickKey9},
            {"QuickKey10", MWInput::A_QuickKey10},
            {"QuickKeysMenu", MWInput::A_QuickKeysMenu},

            {"ToggleHUD", MWInput::A_ToggleHUD},
            {"ToggleDebug", MWInput::A_ToggleDebug},

            {"ZoomIn", MWInput::A_ZoomIn},
            {"ZoomOut", MWInput::A_ZoomOut}
        }));

        api["CONTROL_SWITCH"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, std::string_view>({
            {"Controls", "playercontrols"},
            {"Fighting", "playerfighting"},
            {"Jumping", "playerjumping"},
            {"Looking", "playerlooking"},
            {"Magic", "playermagic"},
            {"ViewMode", "playerviewswitch"},
            {"VanityMode", "vanitymode"}
        }));

        api["CONTROLLER_BUTTON"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, SDL_GameControllerButton>({
            {"A", SDL_CONTROLLER_BUTTON_A},
            {"B", SDL_CONTROLLER_BUTTON_B},
            {"X", SDL_CONTROLLER_BUTTON_X},
            {"Y", SDL_CONTROLLER_BUTTON_Y},
            {"Back", SDL_CONTROLLER_BUTTON_BACK},
            {"Guide", SDL_CONTROLLER_BUTTON_GUIDE},
            {"Start", SDL_CONTROLLER_BUTTON_START},
            {"LeftStick", SDL_CONTROLLER_BUTTON_LEFTSTICK},
            {"RightStick", SDL_CONTROLLER_BUTTON_RIGHTSTICK},
            {"LeftShoulder", SDL_CONTROLLER_BUTTON_LEFTSHOULDER},
            {"RightShoulder", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER},
            {"DPadUp", SDL_CONTROLLER_BUTTON_DPAD_UP},
            {"DPadDown", SDL_CONTROLLER_BUTTON_DPAD_DOWN},
            {"DPadLeft", SDL_CONTROLLER_BUTTON_DPAD_LEFT},
            {"DPadRight", SDL_CONTROLLER_BUTTON_DPAD_RIGHT}
        }));

        api["CONTROLLER_AXIS"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, int>({
            {"LeftX", SDL_CONTROLLER_AXIS_LEFTX},
            {"LeftY", SDL_CONTROLLER_AXIS_LEFTY},
            {"RightX", SDL_CONTROLLER_AXIS_RIGHTX},
            {"RightY", SDL_CONTROLLER_AXIS_RIGHTY},
            {"TriggerLeft", SDL_CONTROLLER_AXIS_TRIGGERLEFT},
            {"TriggerRight", SDL_CONTROLLER_AXIS_TRIGGERRIGHT},

            {"LookUpDown", SDL_CONTROLLER_AXIS_MAX + MWInput::A_LookUpDown},
            {"LookLeftRight", SDL_CONTROLLER_AXIS_MAX + MWInput::A_LookLeftRight},
            {"MoveForwardBackward", SDL_CONTROLLER_AXIS_MAX + MWInput::A_MoveForwardBackward},
            {"MoveLeftRight", SDL_CONTROLLER_AXIS_MAX + MWInput::A_MoveLeftRight}
        }));

        api["KEY"] = LuaUtil::makeReadOnly(context.mLua->tableFromPairs<std::string_view, SDL_Scancode>({
            {"_0", SDL_SCANCODE_0},
            {"_1", SDL_SCANCODE_1},
            {"_2", SDL_SCANCODE_2},
            {"_3", SDL_SCANCODE_3},
            {"_4", SDL_SCANCODE_4},
            {"_5", SDL_SCANCODE_5},
            {"_6", SDL_SCANCODE_6},
            {"_7", SDL_SCANCODE_7},
            {"_8", SDL_SCANCODE_8},
            {"_9", SDL_SCANCODE_9},

            {"NP_0", SDL_SCANCODE_KP_0},
            {"NP_1", SDL_SCANCODE_KP_1},
            {"NP_2", SDL_SCANCODE_KP_2},
            {"NP_3", SDL_SCANCODE_KP_3},
            {"NP_4", SDL_SCANCODE_KP_4},
            {"NP_5", SDL_SCANCODE_KP_5},
            {"NP_6", SDL_SCANCODE_KP_6},
            {"NP_7", SDL_SCANCODE_KP_7},
            {"NP_8", SDL_SCANCODE_KP_8},
            {"NP_9", SDL_SCANCODE_KP_9},
            {"NP_Divide", SDL_SCANCODE_KP_DIVIDE},
            {"NP_Enter", SDL_SCANCODE_KP_ENTER},
            {"NP_Minus", SDL_SCANCODE_KP_MINUS},
            {"NP_Multiply", SDL_SCANCODE_KP_MULTIPLY},
            {"NP_Delete", SDL_SCANCODE_KP_PERIOD},
            {"NP_Plus", SDL_SCANCODE_KP_PLUS},

            {"F1", SDL_SCANCODE_F1},
            {"F2", SDL_SCANCODE_F2},
            {"F3", SDL_SCANCODE_F3},
            {"F4", SDL_SCANCODE_F4},
            {"F5", SDL_SCANCODE_F5},
            {"F6", SDL_SCANCODE_F6},
            {"F7", SDL_SCANCODE_F7},
            {"F8", SDL_SCANCODE_F8},
            {"F9", SDL_SCANCODE_F9},
            {"F10", SDL_SCANCODE_F10},
            {"F11", SDL_SCANCODE_F11},
            {"F12", SDL_SCANCODE_F12},

            {"A", SDL_SCANCODE_A},
            {"B", SDL_SCANCODE_B},
            {"C", SDL_SCANCODE_C},
            {"D", SDL_SCANCODE_D},
            {"E", SDL_SCANCODE_E},
            {"F", SDL_SCANCODE_F},
            {"G", SDL_SCANCODE_G},
            {"H", SDL_SCANCODE_H},
            {"I", SDL_SCANCODE_I},
            {"J", SDL_SCANCODE_J},
            {"K", SDL_SCANCODE_K},
            {"L", SDL_SCANCODE_L},
            {"M", SDL_SCANCODE_M},
            {"N", SDL_SCANCODE_N},
            {"O", SDL_SCANCODE_O},
            {"P", SDL_SCANCODE_P},
            {"Q", SDL_SCANCODE_Q},
            {"R", SDL_SCANCODE_R},
            {"S", SDL_SCANCODE_S},
            {"T", SDL_SCANCODE_T},
            {"U", SDL_SCANCODE_U},
            {"V", SDL_SCANCODE_V},
            {"W", SDL_SCANCODE_W},
            {"X", SDL_SCANCODE_X},
            {"Y", SDL_SCANCODE_Y},
            {"Z", SDL_SCANCODE_Z},

            {"LeftArrow", SDL_SCANCODE_LEFT},
            {"RightArrow", SDL_SCANCODE_RIGHT},
            {"UpArrow", SDL_SCANCODE_UP},
            {"DownArrow", SDL_SCANCODE_DOWN},

            {"LeftAlt", SDL_SCANCODE_LALT},
            {"LeftCtrl", SDL_SCANCODE_LCTRL},
            {"LeftBracket", SDL_SCANCODE_LEFTBRACKET},
            {"LeftSuper", SDL_SCANCODE_LGUI},
            {"LeftShift", SDL_SCANCODE_LSHIFT},
            {"RightAlt", SDL_SCANCODE_RALT},
            {"RightCtrl", SDL_SCANCODE_RCTRL},
            {"RightSuper", SDL_SCANCODE_RGUI},
            {"RightBracket", SDL_SCANCODE_RIGHTBRACKET},
            {"RightShift", SDL_SCANCODE_RSHIFT},

            {"BackSlash", SDL_SCANCODE_BACKSLASH},
            {"Backspace", SDL_SCANCODE_BACKSPACE},
            {"CapsLock", SDL_SCANCODE_CAPSLOCK},
            {"Comma", SDL_SCANCODE_COMMA},
            {"Delete", SDL_SCANCODE_DELETE},
            {"End", SDL_SCANCODE_END},
            {"Enter", SDL_SCANCODE_RETURN},
            {"Equals", SDL_SCANCODE_EQUALS},
            {"Escape", SDL_SCANCODE_ESCAPE},
            {"Home", SDL_SCANCODE_HOME},
            {"Insert", SDL_SCANCODE_INSERT},
            {"Minus", SDL_SCANCODE_MINUS},
            {"NumLock", SDL_SCANCODE_NUMLOCKCLEAR},
            {"PageDown", SDL_SCANCODE_PAGEDOWN},
            {"PageUp", SDL_SCANCODE_PAGEUP},
            {"Pause", SDL_SCANCODE_PAUSE},
            {"PrintScreen", SDL_SCANCODE_PRINTSCREEN},
            {"ScrollLock", SDL_SCANCODE_SCROLLLOCK},
            {"Semicolon", SDL_SCANCODE_SEMICOLON},
            {"Slash", SDL_SCANCODE_SLASH},
            {"Space", SDL_SCANCODE_SPACE},
            {"Tab", SDL_SCANCODE_TAB}
        }));

        return LuaUtil::makeReadOnly(api);
    }

}
