#ifndef MWGUI_WINDOWMANAGER_H
#define MWGUI_WINDOWMANAGER_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.

   MyGUI should be initialized separately before creating instances of
   this class.
 */

namespace MyGUI
{
  class Gui;
}

namespace Compiler
{
    class Extensions;
}

namespace MWWorld
{
    class Environment;
}

namespace MWGui
{
  class HUD;
  class MapWindow;
  class MainMenu;
  class StatsWindow;
  class Console;

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
      GM_Review
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

  class WindowManager
  {
    HUD *hud;
    MapWindow *map;
    MainMenu *menu;
    StatsWindow *stats;
    Console *console;

    MyGUI::Gui *gui;

    // Current gui mode
    GuiMode mode;

    // Currently shown windows in inventory mode
    GuiWindow shown;

    /* Currently ALLOWED windows in inventory mode. This is used at
       the start of the game, when windows are enabled one by one
       through script commands. You can manipulate this through using
       allow() and disableAll().

       The setting should also affect visibility of certain HUD
       elements, but this is not done yet.
     */
    GuiWindow allowed;

    // Update visibility of all windows based on mode, shown and
    // allowed settings.
    void updateVisible();

  public:
    /// The constructor needs the main Gui object
    WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
        const Compiler::Extensions& extensions);
    virtual ~WindowManager();

    void setMode(GuiMode newMode)
    {
      mode = newMode;
      updateVisible();
    }

    GuiMode getMode() const { return mode; }

    // Everything that is not game mode is considered "gui mode"
    bool isGuiMode() const { return getMode() != GM_Game; }

    // Disallow all inventory mode windows
    void disallowAll()
    {
      allowed = GW_None;
      updateVisible();
    }

    // Allow one or more windows
    void allow(GuiWindow wnd)
    {
      allowed = (GuiWindow)(allowed | wnd);
      updateVisible();
    }

    MyGUI::Gui* getGui() const { return gui; }
  };
}
#endif
