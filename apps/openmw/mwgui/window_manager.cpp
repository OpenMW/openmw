#include "window_manager.hpp"
#include "mw_layouts.hpp"

#include "console.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

WindowManager::WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
    const Compiler::Extensions& extensions, bool newGame)
  : gui(_gui), mode(GM_Game), shown(GW_ALL), allowed(newGame ? GW_None : GW_ALL)
{
  // Get size info from the Gui object
  assert(gui);
  int w = gui->getViewWidth();
  int h = gui->getViewHeight();

  hud = new HUD(w,h);
  menu = new MainMenu(w,h);
  map = new MapWindow();
  stats = new StatsWindow (environment.mWorld->getStore());
  console = new Console(w,h, environment, extensions);

  // The HUD is always on
  hud->setVisible(true);

  // Set up visibility
  updateVisible();
}

WindowManager::~WindowManager()
{
  delete console;
  delete hud;
  delete map;
  delete menu;
  delete stats;
}

void WindowManager::updateVisible()
{
  // Start out by hiding everything except the HUD
  map->setVisible(false);
  menu->setVisible(false);
  stats->setVisible(false);
  console->disable();

  // Mouse is visible whenever we're not in game mode
  gui->setVisiblePointer(isGuiMode());

  // If in game mode, don't show anything.
  if(mode == GM_Game)
    {
      return;
    }

  if(mode == GM_MainMenu)
    {
      // Enable the main menu
      menu->setVisible(true);
      return;
    }

  if(mode == GM_Console)
    {
      console->enable();
      return;
    }

  if(mode == GM_Inventory)
    {
      // Ah, inventory mode. First, compute the effective set of
      // windows to show. This is controlled both by what windows the
      // user has opened/closed (the 'shown' variable) and by what
      // windows we are allowed to show (the 'allowed' var.)
      int eff = shown & allowed;

      // Show the windows we want
      map   -> setVisible( eff & GW_Map );
      stats -> setVisible( eff & GW_Stats );
      return;
    }

  // All other modes are ignored
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    stats->setValue (id, value);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    stats->setValue (id, value);
    hud->setValue (id, value);
}

void WindowManager::messageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    std::cout << "message box: " << message << std::endl;

    if (!buttons.empty())
    {
        std::cout << "buttons: ";
        std::copy (buttons.begin(), buttons.end(), std::ostream_iterator<std::string> (std::cout, ", "));
        std::cout << std::endl;
    }
}
