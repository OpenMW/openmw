#ifndef MWGUI_MODE_H
#define MWGUI_MODE_H

namespace MWGui
{
  enum GuiMode
    {
      GM_Game,          // Game mode, only HUD
      GM_Inventory,     // Inventory mode
      GM_MainMenu,      // Main menu mode

      GM_Console,       // Console mode

      // None of the following are implemented yet

      GM_Dialogue,      // NPC interaction
      GM_Barter,
      GM_Rest,
      // .. more here ..

      // Startup character creation dialogs
      GM_Name,
      GM_Race,
      GM_Birth,
      GM_Class,
      GM_ClassGenerate,
      GM_ClassPick,
      GM_ClassCreate,
      GM_Review,
      
      // interactive MessageBox
      GM_InterMessageBox
    };

  // Windows shown in inventory mode
  enum GuiWindow
    {
      GW_None           = 0,

      GW_Map            = 0x01,
      GW_Inventory      = 0x02,
      GW_Magic          = 0x04,
      GW_Stats          = 0x08,

      GW_ALL            = 0xFF
    };
}

#endif
