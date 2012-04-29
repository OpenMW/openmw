#ifndef MWGUI_WINDOWMANAGER_H
#define MWGUI_WINDOWMANAGER_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.

   MyGUI should be initialized separately before creating instances of
   this class.
**/

#include <string>
#include <vector>
#include <set>

#include <components/esm_store/store.hpp>
#include <openengine/ogre/renderer.hpp>
#include <openengine/gui/manager.hpp>
#include "../mwmechanics/stat.hpp"
#include "../mwworld/ptr.hpp"
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
    class World;
}

namespace MWMechanics
{
    class MechanicsManager;
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
  class WindowBase;
  class HUD;
  class MapWindow;
  class MainMenu;
  class StatsWindow;
  class InventoryWindow;
  class Console;
  class JournalWindow;
  class CharacterCreation;

  class TextInputDialog;
  class InfoBoxDialog;
  class DialogueWindow;
  class MessageBoxManager;

  struct ClassPoint
  {
      const char *id;
      // Specialization points to match, in order: Stealth, Combat, Magic
      // Note: Order is taken from http://www.uesp.net/wiki/Morrowind:Class_Quiz
      unsigned int points[3];
  };

  class WindowManager
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;
    typedef std::vector<int> SkillList;

    WindowManager(const Compiler::Extensions& extensions, int fpsLevel, bool newGame, OEngine::Render::OgreRenderer *mOgre, const std::string logpath);
    virtual ~WindowManager();

    void setGuiMode(GuiMode newMode);

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

    bool isGuiMode() const { return getMode() != GM_Game; } // Everything that is not game mode is considered "gui mode"

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

    MWGui::DialogueWindow* getDialogueWindow() {return dialogueWindow;}

    MyGUI::Gui* getGui() const { return gui; }

    void wmUpdateFps(float fps, size_t triangleCount, size_t batchCount)
    {
        mFPS = fps;
        mTriangleCount = triangleCount;
        mBatchCount = batchCount;
    }

//    MWMechanics::DynamicStat<int> getValue(const std::string& id);

    ///< Set value for the given ID.
    void setValue (const std::string& id, const MWMechanics::Stat<int>& value);
    void setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value);
    void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
    void setValue (const std::string& id, const std::string& value);
    void setValue (const std::string& id, int value);

    void setPlayerClass (const ESM::Class &class_);                        ///< set current class of player
    void configureSkills (const SkillList& major, const SkillList& minor); ///< configure skill groups, each set contains the skill ID for that group.
    void setFactions (const FactionList& factions);                        ///< set faction and rank to display on stat window, use an empty vector to disable
    void setBirthSign (const std::string &signId);                         ///< set birth sign to display on stat window, use an empty string to disable.
    void setReputation (int reputation);                                   ///< set the current reputation value
    void setBounty (int bounty);                                           ///< set the current bounty value
    void updateSkillArea();                                                ///< update display of skills, factions, birth sign, reputation and bounty

    void changeCell(MWWorld::Ptr::CellStore* cell); ///< change the active cell
    void setPlayerPos(const float x, const float y); ///< set player position in map space
    void setPlayerDir(const float x, const float y); ///< set player view direction in map space

    void toggleFogOfWar();

    int toggleFps();
    ///< toggle fps display @return resulting fps level

    void setInteriorMapTexture(const int x, const int y);
    ///< set the index of the map texture that should be used (for interiors)

    // sets the visibility of the hud health/magicka/stamina bars
    void setHMSVisibility(bool visible);
    // sets the visibility of the hud minimap
    void setMinimapVisibility(bool visible);

    template<typename T>
    void removeDialog(T*& dialog); ///< Casts to OEngine::GUI::Layout and calls removeDialog, then resets pointer to nullptr.
    void removeDialog(OEngine::GUI::Layout* dialog); ///< Hides dialog and schedules dialog to be deleted.

    void messageBox (const std::string& message, const std::vector<std::string>& buttons);
    int readPressedButton (); ///< returns the index of the pressed button or -1 if no button was pressed (->MessageBoxmanager->InteractiveMessageBox)

    void onFrame (float frameDuration);

    /**
     * Fetches a GMST string from the store, if there is no setting with the given
     * ID or it is not a string the default string is returned.
     *
     * @param id Identifier for the GMST setting, e.g. "aName"
     * @param default Default value if the GMST setting cannot be used.
     */
    const std::string &getGameSettingString(const std::string &id, const std::string &default_);

    const ESMS::ESMStore& getStore() const;

  private:
    OEngine::GUI::MyGUIManager *mGuiManager;
    HUD *hud;
    MapWindow *map;
    MainMenu *menu;
    StatsWindow *stats;
    MessageBoxManager *mMessageBoxManager;
    Console *console;
    JournalWindow* mJournal;
    DialogueWindow *dialogueWindow;

    CharacterCreation* mCharGen;

    // Various stats about player as needed by window manager
    ESM::Class playerClass;
    std::string playerName;
    std::string playerRaceId;
    std::string playerBirthSignId;
    std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> > playerAttributes;
    SkillList playerMajorSkills, playerMinorSkills;
    std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> > playerSkillValues;
    MWMechanics::DynamicStat<int> playerHealth, playerMagicka, playerFatigue;


    MyGUI::Gui *gui; // Gui
    GuiMode mode; // Current gui mode
    GuiMode nextMode; // Next mode to activate in update()
    bool needModeChange; //Whether a mode change is needed in update() [will use nextMode]

    std::vector<OEngine::GUI::Layout*> garbageDialogs;
    void cleanupGarbage();

    GuiWindow shown; // Currently shown windows in inventory mode

    /* Currently ALLOWED windows in inventory mode. This is used at
       the start of the game, when windows are enabled one by one
       through script commands. You can manipulate this through using
       allow() and disableAll().

       The setting should also affect visibility of certain HUD
       elements, but this is not done yet.
     */
    GuiWindow allowed;

    void updateVisible(); // Update visibility of all windows based on mode, shown and allowed settings

    int showFPSLevel;
    float mFPS;
    size_t mTriangleCount;
    size_t mBatchCount;

    void onDialogueWindowBye();
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
