#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwinput/inputmanager.hpp"

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
#if 0
  inventory = new InventoryWindow ();
#endif
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
#if 0
  delete inventory;
#endif

  delete nameDialog;
  delete raceDialog;
}

void WindowManager::updateVisible()
{
  // Start out by hiding everything except the HUD
  map->setVisible(false);
  menu->setVisible(false);
  stats->setVisible(false);
#if 0
  inventory->setVisible(false);
#endif
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
          nameDialog = new TextInputDialog(environment, gui->getViewSize());

      std::string sName = getGameSettingString("sName", "Name");
      nameDialog->setTextLabel(sName);
      nameDialog->setNextButtonShow(nameChosen);
      nameDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onNameDialogDone);
      nameDialog->open();
      return;
  }

  if (mode == GM_Race)
  {
      if (!raceDialog)
          raceDialog = new RaceDialog(environment, gui->getViewSize());
      raceDialog->setNextButtonShow(raceChosen);
      raceDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onRaceDialogDone);
      raceDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onRaceDialogBack);
      raceDialog->open();
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
      map   -> setVisible( (eff & GW_Map) != 0 );
      stats -> setVisible( (eff & GW_Stats) != 0 );
#if 0
//      inventory -> setVisible( eff & GW_Inventory );
#endif
      return;
    }

  // Unsupported mode, switch back to game
  // Note: The call will eventually end up this method again but
  // will stop at the check if(mode == GM_Game) above.
  environment.mInputManager->setGuiMode(GM_Game);
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

const std::string &WindowManager::getGameSettingString(const std::string &id, const std::string &default)
{
    const ESM::GameSetting *setting = environment.mWorld->getStore().gameSettings.search(id);
    if (setting && setting->type == ESM::VT_String)
        return setting->str;
    return default;
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
    nameDialog->eventDone = MWGui::TextInputDialog::EventHandle_Void();

    bool goNext = nameChosen; // Go to next dialog if name was previously chosen
    nameChosen = true;
    if (nameDialog)
    {
        nameDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerName(nameDialog->getTextInput());
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Race);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogDone()
{
    raceDialog->eventDone = MWGui::RaceDialog::EventHandle_Void();

    bool goNext = raceChosen; // Go to next dialog if race was previously chosen
    raceChosen = true;
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRaceId(), raceDialog->getGender() == RaceDialog::GM_Male);
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Class);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogBack()
{
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRaceId(), raceDialog->getGender() == RaceDialog::GM_Male);
    }

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Name);
}
