#include "luabindings.hpp"

#include <SDL_events.h>
#include <SDL_gamecontroller.h>

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
        keyEvent["symbol"] = sol::readonly_property([](const SDL_Keysym& e) { return std::string(1, static_cast<char>(e.sym)); });
        keyEvent["code"] = sol::readonly_property([](const SDL_Keysym& e) -> int { return e.sym; });
        keyEvent["modifiers"] = sol::readonly_property([](const SDL_Keysym& e) -> int { return e.mod; });
        keyEvent["withShift"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_SHIFT; });
        keyEvent["withCtrl"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_CTRL; });
        keyEvent["withAlt"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_ALT; });
        keyEvent["withSuper"] = sol::readonly_property([](const SDL_Keysym& e) -> bool { return e.mod & KMOD_GUI; });

        MWBase::InputManager* input = MWBase::Environment::get().getInputManager();
        sol::table api(context.mLua->sol(), sol::create);

        api["isIdle"] = [input]() { return input->isIdle(); };
        api["isActionPressed"] = [input](int action) { return input->actionIsActive(action); };
        api["isMouseButtonPressed"] = [input](int button) -> bool
        {
            return input->getMouseButtonsState() & (1 << (button - 1));
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

        api["getControlSwitch"] = [input](const std::string& key) { return input->getControlSwitch(key); };
        api["setControlSwitch"] = [input](const std::string& key, bool v) { input->toggleControlSwitch(key, v); };

        api["ACTION"] = context.mLua->makeReadOnly(context.mLua->sol().create_table_with(
            "GameMenu", MWInput::A_GameMenu,
            "Screenshot", MWInput::A_Screenshot,
            "Inventory", MWInput::A_Inventory,
            "Console", MWInput::A_Console,

            "MoveLeft", MWInput::A_MoveLeft,
            "MoveRight", MWInput::A_MoveRight,
            "MoveForward", MWInput::A_MoveForward,
            "MoveBackward", MWInput::A_MoveBackward,

            "Activate", MWInput::A_Activate,
            "Use", MWInput::A_Use,
            "Jump", MWInput::A_Jump,
            "AutoMove", MWInput::A_AutoMove,
            "Rest", MWInput::A_Rest,
            "Journal", MWInput::A_Journal,
            "Weapon", MWInput::A_Weapon,
            "Spell", MWInput::A_Spell,
            "Run", MWInput::A_Run,
            "CycleSpellLeft", MWInput::A_CycleSpellLeft,
            "CycleSpellRight", MWInput::A_CycleSpellRight,
            "CycleWeaponLeft", MWInput::A_CycleWeaponLeft,
            "CycleWeaponRight", MWInput::A_CycleWeaponRight,
            "ToggleSneak", MWInput::A_ToggleSneak,
            "AlwaysRun", MWInput::A_AlwaysRun,
            "Sneak", MWInput::A_Sneak,

            "QuickSave", MWInput::A_QuickSave,
            "QuickLoad", MWInput::A_QuickLoad,
            "QuickMenu", MWInput::A_QuickMenu,
            "ToggleWeapon", MWInput::A_ToggleWeapon,
            "ToggleSpell", MWInput::A_ToggleSpell,
            "TogglePOV", MWInput::A_TogglePOV,

            "QuickKey1", MWInput::A_QuickKey1,
            "QuickKey2", MWInput::A_QuickKey2,
            "QuickKey3", MWInput::A_QuickKey3,
            "QuickKey4", MWInput::A_QuickKey4,
            "QuickKey5", MWInput::A_QuickKey5,
            "QuickKey6", MWInput::A_QuickKey6,
            "QuickKey7", MWInput::A_QuickKey7,
            "QuickKey8", MWInput::A_QuickKey8,
            "QuickKey9", MWInput::A_QuickKey9,
            "QuickKey10", MWInput::A_QuickKey10,
            "QuickKeysMenu", MWInput::A_QuickKeysMenu,

            "ToggleHUD", MWInput::A_ToggleHUD,
            "ToggleDebug", MWInput::A_ToggleDebug,

            "ZoomIn", MWInput::A_ZoomIn,
            "ZoomOut", MWInput::A_ZoomOut
        ));

        api["CONTROL_SWITCH"] = context.mLua->makeReadOnly(context.mLua->sol().create_table_with(
            "Controls", "playercontrols",
            "Fighting", "playerfighting",
            "Jumping", "playerjumping",
            "Looking", "playerlooking",
            "Magic", "playermagic",
            "ViewMode", "playerviewswitch",
            "VanityMode", "vanitymode"
        ));

        api["CONTROLLER_BUTTON"] = context.mLua->makeReadOnly(context.mLua->sol().create_table_with(
            "A", SDL_CONTROLLER_BUTTON_A,
            "B", SDL_CONTROLLER_BUTTON_B,
            "X", SDL_CONTROLLER_BUTTON_X,
            "Y", SDL_CONTROLLER_BUTTON_Y,
            "Back", SDL_CONTROLLER_BUTTON_BACK,
            "Guide", SDL_CONTROLLER_BUTTON_GUIDE,
            "Start", SDL_CONTROLLER_BUTTON_START,
            "LeftStick", SDL_CONTROLLER_BUTTON_LEFTSTICK,
            "RightStick", SDL_CONTROLLER_BUTTON_RIGHTSTICK,
            "LeftShoulder", SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
            "RightShoulder", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            "DPadUp", SDL_CONTROLLER_BUTTON_DPAD_UP,
            "DPadDown", SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            "DPadLeft", SDL_CONTROLLER_BUTTON_DPAD_LEFT,
            "DPadRight", SDL_CONTROLLER_BUTTON_DPAD_RIGHT
        ));

        api["CONTROLLER_AXIS"] = context.mLua->makeReadOnly(context.mLua->sol().create_table_with(
            "LeftX", SDL_CONTROLLER_AXIS_LEFTX,
            "LeftY", SDL_CONTROLLER_AXIS_LEFTY,
            "RightX", SDL_CONTROLLER_AXIS_RIGHTX,
            "RightY", SDL_CONTROLLER_AXIS_RIGHTY,
            "TriggerLeft", SDL_CONTROLLER_AXIS_TRIGGERLEFT,
            "TriggerRight", SDL_CONTROLLER_AXIS_TRIGGERRIGHT,

            "LookUpDown", SDL_CONTROLLER_AXIS_MAX + MWInput::A_LookUpDown,
            "LookLeftRight", SDL_CONTROLLER_AXIS_MAX + MWInput::A_LookLeftRight,
            "MoveForwardBackward", SDL_CONTROLLER_AXIS_MAX + MWInput::A_MoveForwardBackward,
            "MoveLeftRight", SDL_CONTROLLER_AXIS_MAX + MWInput::A_MoveLeftRight
        ));

        return context.mLua->makeReadOnly(api);
    }

}
