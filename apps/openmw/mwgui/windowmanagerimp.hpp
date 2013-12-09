#ifndef MWGUI_WINDOWMANAGERIMP_H
#define MWGUI_WINDOWMANAGERIMP_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.

   MyGUI should be initialized separately before creating instances of
   this class.
**/

#include "../mwbase/windowmanager.hpp"

namespace MyGUI
{
    class Gui;
    class Widget;
    class Window;
    class UString;
}

namespace Compiler
{
    class Extensions;
}

namespace Translation
{
    class Storage;
}

namespace OEngine
{
    namespace GUI
    {
        class Layout;
        class MyGUIManager;
    }

    namespace Render
    {
        class OgreRenderer;
    }
}

namespace SFO
{
    class CursorManager;
}

namespace MWGui
{
  class WindowBase;
  class HUD;
  class MapWindow;
  class MainMenu;
  class StatsWindow;
  class InventoryWindow;
  class JournalWindow;
  class CharacterCreation;
  class DragAndDrop;
  class ToolTips;
  class TextInputDialog;
  class InfoBoxDialog;
  class MessageBoxManager;
  class SettingsWindow;
  class AlchemyWindow;
  class QuickKeysMenu;
  class LoadingScreen;
  class LevelupDialog;
  class WaitDialog;
  class SpellCreationDialog;
  class EnchantingDialog;
  class TrainingWindow;
  class Cursor;
  class SpellIcons;
  class MerchantRepair;
  class Repair;
  class SoulgemDialog;
  class Recharge;
  class CompanionWindow;

  class WindowManager : public MWBase::WindowManager
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;

    WindowManager(const Compiler::Extensions& extensions, int fpsLevel,
                  OEngine::Render::OgreRenderer *mOgre, const std::string& logpath,
                  const std::string& cacheDir, bool consoleOnlyScripts,
                  Translation::Storage& translationDataStorage, ToUTF8::FromType encoding);
    virtual ~WindowManager();

    void initUI();
    void renderWorldMap();

    virtual Loading::Listener* getLoadingScreen();

    /**
     * Should be called each frame to update windows/gui elements.
     * This could mean updating sizes of gui elements or opening
     * new dialogs.
     */
    virtual void update();

    virtual void setKeyFocusWidget (MyGUI::Widget* widget);

    virtual void setNewGame(bool newgame);

    virtual void pushGuiMode(GuiMode mode);
    virtual void popGuiMode();
    virtual void removeGuiMode(GuiMode mode); ///< can be anywhere in the stack

    virtual GuiMode getMode() const;
    virtual bool containsMode(GuiMode mode) const;

    virtual bool isGuiMode() const;

    virtual bool isConsoleMode() const;

    virtual void toggleVisible(GuiWindow wnd);

    virtual void forceHide(MWGui::GuiWindow wnd);
    virtual void unsetForceHide(MWGui::GuiWindow wnd);

    // Disallow all inventory mode windows
    virtual void disallowAll();

    // Allow one or more windows
    virtual void allow(GuiWindow wnd);

    virtual bool isAllowed(GuiWindow wnd) const;

    /// \todo investigate, if we really need to expose every single lousy UI element to the outside world
    virtual MWGui::DialogueWindow* getDialogueWindow();
    virtual MWGui::ContainerWindow* getContainerWindow();
    virtual MWGui::InventoryWindow* getInventoryWindow();
    virtual MWGui::BookWindow* getBookWindow();
    virtual MWGui::ScrollWindow* getScrollWindow();
    virtual MWGui::CountDialog* getCountDialog();
    virtual MWGui::ConfirmationDialog* getConfirmationDialog();
    virtual MWGui::TradeWindow* getTradeWindow();
    virtual MWGui::SpellBuyingWindow* getSpellBuyingWindow();
    virtual MWGui::TravelWindow* getTravelWindow();
    virtual MWGui::SpellWindow* getSpellWindow();
    virtual MWGui::Console* getConsole();

    virtual MyGUI::Gui* getGui() const;

    virtual void wmUpdateFps(float fps, unsigned int triangleCount, unsigned int batchCount);

    ///< Set value for the given ID.
    virtual void setValue (const std::string& id, const MWMechanics::Stat<int>& value);
    virtual void setValue (int parSkill, const MWMechanics::Stat<float>& value);
    virtual void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value);
    virtual void setValue (const std::string& id, const std::string& value);
    virtual void setValue (const std::string& id, int value);

    /// Set time left for the player to start drowning (update the drowning bar)
    /// @param time value from [0,20]
    virtual void setDrowningTimeLeft (float time);

    virtual void setPlayerClass (const ESM::Class &class_);                        ///< set current class of player
    virtual void configureSkills (const SkillList& major, const SkillList& minor); ///< configure skill groups, each set contains the skill ID for that group.
    virtual void setReputation (int reputation);                                   ///< set the current reputation value
    virtual void setBounty (int bounty);                                           ///< set the current bounty value
    virtual void updateSkillArea();                                                ///< update display of skills, factions, birth sign, reputation and bounty

    virtual void changeCell(MWWorld::CellStore* cell); ///< change the active cell
    virtual void setPlayerPos(const float x, const float y); ///< set player position in map space
    virtual void setPlayerDir(const float x, const float y); ///< set player view direction in map space

    virtual void setFocusObject(const MWWorld::Ptr& focus);
    virtual void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y);

    virtual void getMousePosition(int &x, int &y);
    virtual void getMousePosition(float &x, float &y);
    virtual void setDragDrop(bool dragDrop);
    virtual bool getWorldMouseOver();

    virtual void toggleFogOfWar();
    virtual void toggleFullHelp(); ///< show extra info in item tooltips (owner, script)
    virtual bool getFullHelp() const;

    virtual void setInteriorMapTexture(const int x, const int y);
    ///< set the index of the map texture that should be used (for interiors)

    /// sets the visibility of the drowning bar
    virtual void setDrowningBarVisibility(bool visible);

    // sets the visibility of the hud health/magicka/stamina bars
    virtual void setHMSVisibility(bool visible);
    // sets the visibility of the hud minimap
    virtual void setMinimapVisibility(bool visible);
    virtual void setWeaponVisibility(bool visible);
    virtual void setSpellVisibility(bool visible);
    virtual void setSneakVisibility(bool visible);

    virtual void activateQuickKey  (int index);

    virtual void setSelectedSpell(const std::string& spellId, int successChancePercent);
    virtual void setSelectedEnchantItem(const MWWorld::Ptr& item);
    virtual void setSelectedWeapon(const MWWorld::Ptr& item);
    virtual void unsetSelectedSpell();
    virtual void unsetSelectedWeapon();

    virtual void showCrosshair(bool show);
    virtual bool getSubtitlesEnabled();
    virtual void toggleHud();

    virtual void disallowMouse();
    virtual void allowMouse();
    virtual void notifyInputActionBound();

    virtual void addVisitedLocation(const std::string& name, int x, int y);

    virtual void removeDialog(OEngine::GUI::Layout* dialog); ///< Hides dialog and schedules dialog to be deleted.

    virtual void messageBox (const std::string& message, const std::vector<std::string>& buttons = std::vector<std::string>(), bool showInDialogueModeOnly = false);
    virtual void staticMessageBox(const std::string& message);
    virtual void removeStaticMessageBox();
    virtual void enterPressed ();
    virtual void activateKeyPressed ();
    virtual int readPressedButton (); ///< returns the index of the pressed button or -1 if no button was pressed (->MessageBoxmanager->InteractiveMessageBox)

    virtual void onFrame (float frameDuration);

    /// \todo get rid of this stuff. Move it to the respective UI element classes, if needed.
    virtual std::map<int, MWMechanics::Stat<float> > getPlayerSkillValues();
    virtual std::map<int, MWMechanics::Stat<int> > getPlayerAttributeValues();
    virtual SkillList getPlayerMinorSkills();
    virtual SkillList getPlayerMajorSkills();

    /**
     * Fetches a GMST string from the store, if there is no setting with the given
     * ID or it is not a string the default string is returned.
     *
     * @param id Identifier for the GMST setting, e.g. "aName"
     * @param default Default value if the GMST setting cannot be used.
     */
    virtual std::string getGameSettingString(const std::string &id, const std::string &default_);

    virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

    virtual void windowResized(int x, int y);

    virtual void executeInConsole (const std::string& path);

    virtual void enableRest() { mRestAllowed = true; }
    virtual bool getRestEnabled();

    virtual bool getJournalAllowed() { return (mAllowed & GW_Magic); }

    virtual bool getPlayerSleeping();
    virtual void wakeUpPlayer();

    virtual void updatePlayer();

    virtual void showCompanionWindow(MWWorld::Ptr actor);
    virtual void startSpellMaking(MWWorld::Ptr actor);
    virtual void startEnchanting(MWWorld::Ptr actor);
    virtual void startSelfEnchanting(MWWorld::Ptr soulgem);
    virtual void startTraining(MWWorld::Ptr actor);
    virtual void startRepair(MWWorld::Ptr actor);
    virtual void startRepairItem(MWWorld::Ptr item);
    virtual void startRecharge(MWWorld::Ptr soulgem);

    virtual void frameStarted(float dt);

    virtual void showSoulgemDialog (MWWorld::Ptr item);

    virtual void changePointer (const std::string& name);

    virtual void setEnemy (const MWWorld::Ptr& enemy);

    virtual const Translation::Storage& getTranslationDataStorage() const;

    void onSoulgemDialogButtonPressed (int button);

    virtual bool getCursorVisible();

  private:
    bool mConsoleOnlyScripts;

    std::map<MyGUI::Window*, std::string> mTrackedWindows;
    void trackWindow(OEngine::GUI::Layout* layout, const std::string& name);
    void onWindowChangeCoord(MyGUI::Window* _sender);

    OEngine::GUI::MyGUIManager *mGuiManager;
    OEngine::Render::OgreRenderer *mRendering;
    HUD *mHud;
    MapWindow *mMap;
    MainMenu *mMenu;
    ToolTips *mToolTips;
    StatsWindow *mStatsWindow;
    MessageBoxManager *mMessageBoxManager;
    Console *mConsole;
    JournalWindow* mJournal;
    DialogueWindow *mDialogueWindow;
    ContainerWindow *mContainerWindow;
    DragAndDrop* mDragAndDrop;
    InventoryWindow *mInventoryWindow;
    ScrollWindow* mScrollWindow;
    BookWindow* mBookWindow;
    CountDialog* mCountDialog;
    TradeWindow* mTradeWindow;
    SpellBuyingWindow* mSpellBuyingWindow;
    TravelWindow* mTravelWindow;
    SettingsWindow* mSettingsWindow;
    ConfirmationDialog* mConfirmationDialog;
    AlchemyWindow* mAlchemyWindow;
    SpellWindow* mSpellWindow;
    QuickKeysMenu* mQuickKeysMenu;
    LoadingScreen* mLoadingScreen;
    LevelupDialog* mLevelupDialog;
    WaitDialog* mWaitDialog;
    SpellCreationDialog* mSpellCreationDialog;
    EnchantingDialog* mEnchantingDialog;
    TrainingWindow* mTrainingWindow;
    MerchantRepair* mMerchantRepair;
    SoulgemDialog* mSoulgemDialog;
    Repair* mRepair;
    Recharge* mRecharge;
    CompanionWindow* mCompanionWindow;

    Translation::Storage& mTranslationDataStorage;
    Cursor* mSoftwareCursor;

    CharacterCreation* mCharGen;

    MyGUI::Widget* mInputBlocker;

    bool mCrosshairEnabled;
    bool mSubtitlesEnabled;
    bool mHudEnabled;
    bool mCursorVisible;

    void setCursorVisible(bool visible);

    /// \todo get rid of this stuff. Move it to the respective UI element classes, if needed.
    // Various stats about player as needed by window manager
    std::string mPlayerName;
    std::string mPlayerRaceId;
    std::map<int, MWMechanics::Stat<int> > mPlayerAttributes;
    SkillList mPlayerMajorSkills, mPlayerMinorSkills;
    std::map<int, MWMechanics::Stat<float> > mPlayerSkillValues;

    MyGUI::Gui *mGui; // Gui
    std::vector<GuiMode> mGuiModes;

    SFO::CursorManager* mCursorManager;

    std::vector<OEngine::GUI::Layout*> mGarbageDialogs;
    void cleanupGarbage();

    GuiWindow mShown; // Currently shown windows in inventory mode
    GuiWindow mForceHidden; // Hidden windows (overrides mShown)

    /* Currently ALLOWED windows in inventory mode. This is used at
       the start of the game, when windows are enabled one by one
       through script commands. You can manipulate this through using
       allow() and disableAll().
     */
    GuiWindow mAllowed;
    // is the rest window allowed?
    bool mRestAllowed;

    void updateVisible(); // Update visibility of all windows based on mode, shown and allowed settings

    int mShowFPSLevel;
    float mFPS;
    unsigned int mTriangleCount;
    unsigned int mBatchCount;

    /**
     * Called when MyGUI tries to retrieve a tag. This usually corresponds to a GMST string,
     * so this method will retrieve the GMST with the name \a _tag and place the result in \a _result
     */
    void onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

    void onCursorChange(const std::string& name);
    void onKeyFocusChanged(MyGUI::Widget* widget);
  };
}

#endif
