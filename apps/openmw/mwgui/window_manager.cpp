#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"

#include "console.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

WindowManager::WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
    const Compiler::Extensions& extensions, bool newGame)
  : environment(environment)
  , nameDialog(nullptr)
  , raceDialog(nullptr)
  , nameChosen(false)
  , raceChosen(false)
  , classChosen(false)
  , birthChosen(false)
  , reviewNext(false)
  , gui(_gui)
  , mode(GM_Game)
  , shown(GW_ALL)
  , allowed(newGame ? GW_None : GW_ALL)
{
  // Get size info from the Gui object
  assert(gui);
  int w = gui->getViewSize().width;
  int h = gui->getViewSize().height;

  hud = new HUD(w,h);
  menu = new MainMenu(w,h);
  map = new MapWindow();
  stats = new StatsWindow (environment.mWorld->getStore());
  inventory = new InventoryWindow ();
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
  delete inventory;

  delete nameDialog;
  delete raceDialog;
}

void WindowManager::updateVisible()
{
  // Start out by hiding everything except the HUD
  map->setVisible(false);
  menu->setVisible(false);
  stats->setVisible(false);
  inventory->setVisible(false);
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

  if (mode == GM_Name)
  {
      if (!nameDialog)
          nameDialog = new TextInputDialog(environment, "Name", nameChosen, gui->getViewSize());
      nameDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onNameDialogDone);
      nameDialog->setVisible(true);
      return;
  }

  if (mode == GM_Race)
  {
      if (!raceDialog)
          raceDialog = new RaceDialog (environment, raceChosen);
      raceDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onRaceDialogDone);
      raceDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onRaceDialogBack);
      raceDialog->setVisible(true);
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
//      inventory -> setVisible( eff & GW_Inventory );
      return;
    }

  // All other modes are ignored
  mode = GM_Game;
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

void WindowManager::updateCharacterGeneration()
{
    if (raceDialog)
    {
        // TOOD: Uncomment when methods in mechanics manager is implemented
        //raceDialog->setRace(environment.mMechanicsManager->getPlayerRace());
        //raceDialog->setGender(environment.mMechanicsManager->getPlayerMale() ? RaceDialog::GM_Male : RaceDialog::GM_Female);
        // TODO: Face/Hair
    }
}

void WindowManager::onNameDialogDone()
{
    nameChosen = true;
    if (nameDialog)
    {
        nameDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerName(nameDialog->getTextInput());
    }
    delete nameDialog;
    nameDialog = nullptr;

    updateCharacterGeneration();

    if (reviewNext)
        setMode(GM_Review);
    else if (raceChosen)
        setMode(GM_Race);
}

void WindowManager::onRaceDialogDone()
{
    raceChosen = true;
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRace(), raceDialog->getGender() == RaceDialog::GM_Male);
    }
    delete raceDialog;
    raceDialog = nullptr;

    updateCharacterGeneration();

    if (reviewNext)
        setMode(GM_Review);
    else if (classChosen)
        setMode(GM_Class);
}

void WindowManager::onRaceDialogBack()
{
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRace(), raceDialog->getGender() == RaceDialog::GM_Male);
    }
    delete raceDialog;
    raceDialog = nullptr;

    updateCharacterGeneration();

    setMode(GM_Name);
}
