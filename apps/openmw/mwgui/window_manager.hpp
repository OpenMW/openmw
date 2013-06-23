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

#include <components/esm_store/store.hpp>
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

namespace OEngine
{
    namespace GUI
    {
        class Layout;
    }
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
  class DialogueWindow;
  class ClassChoiceDialog;
  class GenerateClassResultDialog;
  class PickClassDialog;
  class CreateClassDialog;
  class BirthDialog;
  class ReviewDialog;

  class WindowManager
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;
    typedef std::vector<int> SkillList;

  private:
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
    DialogueWindow *dialogueWindow;
    ClassChoiceDialog *classChoiceDialog;
    InfoBoxDialog *generateClassQuestionDialog;
    GenerateClassResultDialog *generateClassResultDialog;
    PickClassDialog *pickClassDialog;
    CreateClassDialog *createClassDialog;
    BirthDialog *birthSignDialog;
    ReviewDialog *reviewDialog;

    // Which dialogs have been shown, controls back/next/ok buttons
    bool nameChosen;
    bool raceChosen;
    bool classChosen;
    bool birthSignChosen;
    bool reviewNext;
    ///< If true then any click on Next will cause the summary to be shown

    // Keeps track of current step in Generate Class dialogs
    unsigned generateClassStep;
    // A counter for each specialization which is increased when an answer is chosen, in order: Stealth, Combat, Magic
    unsigned generateClassSpecializations[3];
    std::string generateClass;

    // Various stats about player as needed by window manager
    std::string playerName;
    ESM::Class playerClass;
    std::string playerRaceId, playerBirthSignId;
    std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> > playerAttributes;
    SkillList playerMajorSkills, playerMinorSkills;
    std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> > playerSkillValues;
    MWMechanics::DynamicStat<int> playerHealth, playerMagicka, playerFatigue;

    // Gui
    MyGUI::Gui *gui;

    // Current gui mode
    GuiMode mode;

    /**
     * Next mode to activate in update().
     */
    GuiMode nextMode;
    /**
     * Whether a mode change is needed in update().
     * Will use @a nextMode as the new mode.
     */
    bool needModeChange;

    std::vector<OEngine::GUI::Layout*> garbageDialogs;
    void cleanupGarbage();

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

    void setGuiMode(GuiMode newMode);

  public:
    /// The constructor needs the main Gui object
    WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
        const Compiler::Extensions& extensions, bool newGame);
    virtual ~WindowManager();

    /**
     * Should be called each frame to update windows/gui elements.
     * This could mean updating sizes of gui elements or opening
     * new dialogs.
     */
    void update();

    void setMode(GuiMode newMode)
    {
      if (newMode==GM_Inventory && allowed==GW_None)
        return;

      mode = newMode;
      updateVisible();
    }
    void setNextMode(GuiMode newMode);

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

    void setPlayerClass (const ESM::Class &class_);
    ///< set current class of player

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

    template<typename T>
    void removeDialog(T*& dialog);
    ///< Casts to OEngine::GUI::Layout and calls removeDialog, then resets pointer to nullptr.

    void removeDialog(OEngine::GUI::Layout* dialog);
    ///< Hides dialog and schedules dialog to be deleted.

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

    void onDialogueWindowBye();

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

    // Character generation: Review dialog
    void onReviewDialogDone();
    void onReviewDialogBack();
    void onNameDialogActivate();
    void onRaceDialogActivate();
    void onClassDialogActivate();
    void onBirthSignDialogActivate();
  };

  template<typename T>
  void WindowManager::removeDialog(T*& dialog)
  {
      OEngine::GUI::Layout *d = static_cast<OEngine::GUI::Layout*>(dialog);
      removeDialog(d);
      dialog = 0;
  }
}

#endif
