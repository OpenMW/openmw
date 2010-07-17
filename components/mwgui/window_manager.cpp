#include "window_manager.hpp"
#include "mw_layouts.hpp"

#include <assert.h>

using namespace MWGui;

WindowManager::WindowManager(MyGUI::Gui *_gui)
  : gui(_gui), mode(GM_Game), shown(GW_ALL), allowed(GW_ALL)
{
  // Get size info from the Gui object
  assert(gui);
  int w = gui->getViewWidth();
  int h = gui->getViewHeight();

  hud = new HUD(w,h);
  menu = new MainMenu(w,h);
  map = new MapWindow();
  stats = new StatsWindow();

  // The HUD is always on
  hud->setVisible(true);

  // Set up visibility
  updateVisible();
}

WindowManager::~WindowManager()
{
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
