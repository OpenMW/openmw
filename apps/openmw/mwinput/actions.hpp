#ifndef MWINPUT_ACTIONS_H
#define MWINPUT_ACTIONS_H

namespace MWInput
{
    enum Actions
    {
        // Action IDs are used in the configuration file input_v3.xml

        A_GameMenu = 0,

        A_Screenshot = 2, // Take a screenshot

        A_Inventory = 3, // Toggle inventory screen
        A_Console = 4, // Toggle console screen

        A_MoveLeft = 5, // Move player left / right
        A_MoveRight = 6,
        A_MoveForward = 7, // Forward / Backward
        A_MoveBackward = 8,

        A_Activate = 9,

        A_Use = 10, // Use weapon, spell, etc.
        A_Jump = 11,
        A_AutoMove = 12, // Toggle Auto-move forward
        A_Rest = 13, // Rest
        A_Journal = 14, // Journal
        A_Run = 17, // Run when held
        A_CycleSpellLeft = 18, // cycling through spells
        A_CycleSpellRight = 19,
        A_CycleWeaponLeft = 20, // Cycling through weapons
        A_CycleWeaponRight = 21,
        A_AlwaysRun = 23, // Toggle Walking/Running
        A_Sneak = 24,

        A_QuickSave = 25,
        A_QuickLoad = 26,
        A_QuickMenu = 27,
        A_ToggleWeapon = 28,
        A_ToggleSpell = 29,

        A_TogglePOV = 30,

        A_QuickKey1 = 31,
        A_QuickKey2 = 32,
        A_QuickKey3 = 33,
        A_QuickKey4 = 34,
        A_QuickKey5 = 35,
        A_QuickKey6 = 36,
        A_QuickKey7 = 37,
        A_QuickKey8 = 38,
        A_QuickKey9 = 39,
        A_QuickKey10 = 40,

        A_QuickKeysMenu = 41,

        A_ToggleHUD = 42,
        A_ToggleDebug = 43,

        A_LookUpDown = 44, // Joystick look
        A_LookLeftRight = 45,
        A_MoveForwardBackward = 46,
        A_MoveLeftRight = 47,

        A_ZoomIn = 48,
        A_ZoomOut = 49,

        A_TogglePostProcessorHUD = 50,

        A_Last // Marker for the last item
    };
}
#endif
