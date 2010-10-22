#ifndef MWGUI_WINDOWMANAGER_H
#define MWGUI_WINDOWMANAGER_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.

   MyGUI should be initialized separately before creating instances of
   this class.
 */

#include <string>
#include <vector>
#include <set>

#include "../mwmechanics/stat.hpp"
#include "mode.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
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
  class InventoryWindow;
  class Console;

  class TextInputDialog;
  class InfoBoxDialog;
  class RaceDialog;
  class ClassChoiceDialog;
  class GenerateClassResultDialog;
  class PickClassDialog;
  class CreateClassDialog;
  class BirthDialog;

  class WindowManager
  {
    MWWorld::Environment& environment;
    HUD *hud;
    MapWindow *map;
    MainMenu *menu;
    StatsWindow *stats;
#if 0
    InventoryWindow *inventory;
#endif
    Console *console;

    // Character creation
    TextInputDialog *nameDialog;
    RaceDialog *raceDialog;
    ClassChoiceDialog *classChoiceDialog;
    InfoBoxDialog *generateClassQuestionDialog;
    GenerateClassResultDialog *generateClassResultDialog;
    PickClassDialog *pickClassDialog;
    CreateClassDialog *createClassDialog;
    BirthDialog *birthSignDialog;

    // Which dialogs have been shown, controls back/next/ok buttons
    bool nameChosen;
    bool raceChosen;
    bool classChosen;
    bool birthSignChosen;
    bool reviewNext;
    ///< If true then any click on Next will cause the summary to be shown

    // Keeps track of current step in Generate Class dialogs
    unsigned generateClassStep;
    std::string generateClass;

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
        const Compiler::Extensions& extensions, bool newGame);
    virtual ~WindowManager();

    void setMode(GuiMode newMode)
    {
      if (newMode==GM_Inventory && allowed==GW_None)
        return;

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

    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;
    typedef std::vector<int> SkillList;

    void setValue (const std::string& id, const MWMechanics::Stat<int>& value);
    ///< Set value for the given ID.

    void setValue (const std::string& id, const MWMechanics::Stat<float>& value);
    ///< Set value for the given ID.

    void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
    ///< Set value for the given ID.

    void setValue (const std::string& id, const std::string& value);
    ///< set value for the given ID.

    void setValue (const std::string& id, int value);
    ///< set value for the given ID.

    void configureSkills (const SkillList& major, const SkillList& minor);
    ///< configure skill groups, each set contains the skill ID for that group.

    void setFactions (const FactionList& factions);
    ///< set faction and rank to display on stat window, use an empty vector to disable

    void setBirthSign (const std::string &signId);
    ///< set birth sign to display on stat window, use an empty string to disable.

    void setReputation (int reputation);
    ///< set the current reputation value

    void setBounty (int bounty);
    ///< set the current bounty value

    void updateSkillArea();
    ///< update display of skills, factions, birth sign, reputation and bounty

    void messageBox (const std::string& message, const std::vector<std::string>& buttons);

    /**
     * Fetches a GMST string from the store, if there is no setting with the given
     * ID or it is not a string the default string is returned.
     *
     * @param id Identifier for the GMST setting, e.g. "aName"
     * @param default Default value if the GMST setting cannot be used.
     */
    const std::string &getGameSettingString(const std::string &id, const std::string &default_);

  private:
    void updateCharacterGeneration();
    void checkCharacterGeneration(GuiMode mode);

    // Character generation: Name dialog
    void onNameDialogDone();

    // Character generation: Race dialog
    void onRaceDialogDone();
    void onRaceDialogBack();

    // Character generation: Choose class process
    void onClassChoice(MyGUI::Widget* _sender, int _index);

    // Character generation: Generate Class
    void showClassQuestionDialog();
    void onClassQuestionChosen(MyGUI::Widget* _sender, int _index);
    void onGenerateClassBack();
    void onGenerateClassDone();

    // Character generation: Pick Class dialog
    void onPickClassDialogDone();
    void onPickClassDialogBack();

    // Character generation: Create Class dialog
    void onCreateClassDialogDone();
    void onCreateClassDialogBack();

    // Character generation: Birth sign dialog
    void onBirthSignDialogDone();
    void onBirthSignDialogBack();
  };
}
#endif
