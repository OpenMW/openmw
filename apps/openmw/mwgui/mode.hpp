#ifndef MWGUI_MODE_H
#define MWGUI_MODE_H

namespace MWGui
{
  enum GuiMode
    {
      GM_None,
      GM_Settings,      // Settings window
      GM_Inventory,     // Inventory mode
      GM_Container,
      GM_Companion,
      GM_MainMenu,      // Main menu mode

      GM_Journal,       // Journal mode

      GM_Scroll,        // Read scroll
      GM_Book,          // Read book
      GM_Alchemy,       // Make potions
      GM_Repair,

      GM_Dialogue,      // NPC interaction
      GM_Barter,
      GM_Rest,
      GM_SpellBuying,
      GM_Travel,
      GM_SpellCreation,
      GM_Enchanting,
      GM_Recharge,
      GM_Training,
      GM_MerchantRepair,

      GM_Levelup,

      // Startup character creation dialogs
      GM_Name,
      GM_Race,
      GM_Birth,
      GM_Class,
      GM_ClassGenerate,
      GM_ClassPick,
      GM_ClassCreate,
      GM_Review,
      
      GM_Loading,
      GM_LoadingWallpaper,
      GM_Jail,

      GM_QuickKeysMenu
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
