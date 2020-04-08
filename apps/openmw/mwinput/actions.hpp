#ifndef MWINPUT_ACTIONS_H
#define MWINPUT_ACTIONS_H

namespace MWInput
{
    enum Actions
    {
        // please add new actions at the bottom, in order to preserve the channel IDs in the key configuration files

        A_GameMenu,

        A_Unused,

        A_Screenshot,               // Take a screenshot

        A_Inventory,                // Toggle inventory screen

        A_Console,                  // Toggle console screen

        A_MoveLeft,                 // Move player left / right
        A_MoveRight,
        A_MoveForward,              // Forward / Backward
        A_MoveBackward,

        A_Activate,

        A_Use,                      //Use weapon, spell, etc.
        A_Jump,
        A_AutoMove,                 //Toggle Auto-move forward
        A_Rest,                     //Rest
        A_Journal,                  //Journal
        A_Weapon,                   //Draw/Sheath weapon
        A_Spell,                    //Ready/Unready Casting
        A_Run,                      //Run when held
        A_CycleSpellLeft,           //cycling through spells
        A_CycleSpellRight,
        A_CycleWeaponLeft,          //Cycling through weapons
        A_CycleWeaponRight,
        A_ToggleSneak,              //Toggles Sneak
        A_AlwaysRun,                //Toggle Walking/Running
        A_Sneak,

        A_QuickSave,
        A_QuickLoad,
        A_QuickMenu,
        A_ToggleWeapon,
        A_ToggleSpell,

        A_TogglePOV,

        A_QuickKey1,
        A_QuickKey2,
        A_QuickKey3,
        A_QuickKey4,
        A_QuickKey5,
        A_QuickKey6,
        A_QuickKey7,
        A_QuickKey8,
        A_QuickKey9,
        A_QuickKey10,

        A_QuickKeysMenu,

        A_ToggleHUD,

        A_ToggleDebug,

        A_LookUpDown,               //Joystick look
        A_LookLeftRight,
        A_MoveForwardBackward,
        A_MoveLeftRight,

        A_ZoomIn,
        A_ZoomOut,

        A_Last                      // Marker for the last item
    };
}
#endif
