#ifndef MWGUI_WINDOWMANAGERIMP_H
#define MWGUI_WINDOWMANAGERIMP_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.
**/

#include <stack>

#include <osg/ref_ptr>

#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"

#include <components/settings/settings.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include "mapwindow.hpp"
#include "textcolours.hpp"

#include <MyGUI_KeyCode.h>
#include <MyGUI_Types.h>

namespace MyGUI
{
    class Gui;
    class Widget;
    class Window;
    class UString;
    class ImageBox;
}

namespace MWWorld
{
    class ESMStore;
}

namespace Compiler
{
    class Extensions;
}

namespace Translation
{
    class Storage;
}

namespace osg
{
    class Group;
}
namespace osgViewer
{
    class Viewer;
}

namespace Resource
{
    class ResourceSystem;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace SDLUtil
{
    class SDLCursorManager;
}

namespace osgMyGUI
{
    class Platform;
}

namespace Gui
{
    class FontLoader;
}

namespace MWRender
{
    class LocalMap;
}

namespace MWGui
{
  class WindowBase;
  class HUD;
  class MapWindow;
  class MainMenu;
  class StatsWindow;
  class InventoryWindow;
  struct JournalWindow;
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
  class SpellIcons;
  class MerchantRepair;
  class Repair;
  class SoulgemDialog;
  class Recharge;
  class CompanionWindow;
  class VideoWidget;
  class WindowModal;
  class ScreenFader;
  class DebugWindow;
  class JailScreen;
  class KeyboardNavigation;

  class WindowManager : public MWBase::WindowManager
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;

    WindowManager(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
                  const std::string& logpath, const std::string& cacheDir, bool consoleOnlyScripts, Translation::Storage& translationDataStorage,
                  ToUTF8::FromType encoding, bool exportFonts, const std::string& versionDescription, const std::string& localPath);
    virtual ~WindowManager();

    /// Set the ESMStore to use for retrieving of GUI-related strings.
    void setStore (const MWWorld::ESMStore& store);

    void initUI();
    virtual void loadUserFonts();

    virtual Loading::Listener* getLoadingScreen();

    /// @note This method will block until the video finishes playing
    /// (and will continually update the window while doing so)
    virtual void playVideo(const std::string& name, bool allowSkipping);

    /// Warning: do not use MyGUI::InputManager::setKeyFocusWidget directly. Instead use this.
    virtual void setKeyFocusWidget (MyGUI::Widget* widget);

    virtual void setNewGame(bool newgame);

    virtual void pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg);
    virtual void pushGuiMode (GuiMode mode);
    virtual void popGuiMode(bool noSound=false);
    virtual void removeGuiMode(GuiMode mode, bool noSound=false); ///< can be anywhere in the stack

    virtual void goToJail(int days);

    virtual GuiMode getMode() const;
    virtual bool containsMode(GuiMode mode) const;

    virtual bool isGuiMode() const;

    virtual bool isConsoleMode() const;

    virtual void toggleVisible(GuiWindow wnd);

    virtual void forceHide(MWGui::GuiWindow wnd);
    virtual void unsetForceHide(MWGui::GuiWindow wnd);

    /// Disallow all inventory mode windows
    virtual void disallowAll();

    /// Allow one or more windows
    virtual void allow(GuiWindow wnd);

    virtual bool isAllowed(GuiWindow wnd) const;

    /// \todo investigate, if we really need to expose every single lousy UI element to the outside world
    virtual MWGui::InventoryWindow* getInventoryWindow();
    virtual MWGui::CountDialog* getCountDialog();
    virtual MWGui::ConfirmationDialog* getConfirmationDialog();
    virtual MWGui::TradeWindow* getTradeWindow();

    /// Make the player use an item, while updating GUI state accordingly
    virtual void useItem(const MWWorld::Ptr& item, bool bypassBeastRestrictions=false);

    virtual void updateSpellWindow();

    virtual void setConsoleSelectedObject(const MWWorld::Ptr& object);

    ///< Set value for the given ID.
    virtual void setValue (const std::string& id, const MWMechanics::AttributeValue& value);
    virtual void setValue (int parSkill, const MWMechanics::SkillValue& value);
    virtual void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value);
    virtual void setValue (const std::string& id, const std::string& value);
    virtual void setValue (const std::string& id, int value);

    /// Set time left for the player to start drowning (update the drowning bar)
    /// @param time time left to start drowning
    /// @param maxTime how long we can be underwater (in total) until drowning starts
    virtual void setDrowningTimeLeft (float time, float maxTime);

    virtual void setPlayerClass (const ESM::Class &class_);                        ///< set current class of player
    virtual void configureSkills (const SkillList& major, const SkillList& minor); ///< configure skill groups, each set contains the skill ID for that group.
    virtual void updateSkillArea();                                                ///< update display of skills, factions, birth sign, reputation and bounty

    virtual void changeCell(const MWWorld::CellStore* cell); ///< change the active cell

    virtual void setFocusObject(const MWWorld::Ptr& focus);
    virtual void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y);

    virtual void getMousePosition(int &x, int &y);
    virtual void getMousePosition(float &x, float &y);
    virtual void setDragDrop(bool dragDrop);
    virtual bool getWorldMouseOver();

    virtual bool toggleFogOfWar();
    virtual bool toggleFullHelp(); ///< show extra info in item tooltips (owner, script)
    virtual bool getFullHelp() const;

    virtual void setActiveMap(int x, int y, bool interior);
    ///< set the indices of the map texture that should be used

    /// sets the visibility of the drowning bar
    virtual void setDrowningBarVisibility(bool visible);

    // sets the visibility of the hud health/magicka/stamina bars
    virtual void setHMSVisibility(bool visible);
    // sets the visibility of the hud minimap
    virtual void setMinimapVisibility(bool visible);
    virtual void setWeaponVisibility(bool visible);
    virtual void setSpellVisibility(bool visible);
    virtual void setSneakVisibility(bool visible);

    /// activate selected quick key
    virtual void activateQuickKey (int index);
    /// update activated quick key state (if action executing was delayed for some reason)
    virtual void updateActivatedQuickKey ();

    virtual std::string getSelectedSpell() { return mSelectedSpell; }
    virtual void setSelectedSpell(const std::string& spellId, int successChancePercent);
    virtual void setSelectedEnchantItem(const MWWorld::Ptr& item);
    virtual const MWWorld::Ptr& getSelectedEnchantItem() const;
    virtual void setSelectedWeapon(const MWWorld::Ptr& item);
    virtual const MWWorld::Ptr& getSelectedWeapon() const;
    virtual int getFontHeight() const;
    virtual void unsetSelectedSpell();
    virtual void unsetSelectedWeapon();

    virtual void showCrosshair(bool show);
    virtual bool getSubtitlesEnabled();

    /// Turn visibility of HUD on or off
    virtual bool toggleHud();

    virtual void disallowMouse();
    virtual void allowMouse();
    virtual void notifyInputActionBound();

    virtual void addVisitedLocation(const std::string& name, int x, int y);

    ///Hides dialog and schedules dialog to be deleted.
    virtual void removeDialog(Layout* dialog);

    ///Gracefully attempts to exit the topmost GUI mode
    virtual void exitCurrentGuiMode();

    virtual void messageBox (const std::string& message, enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible);
    virtual void staticMessageBox(const std::string& message);
    virtual void removeStaticMessageBox();
    virtual void interactiveMessageBox (const std::string& message,
                                        const std::vector<std::string>& buttons = std::vector<std::string>(), bool block=false);

    virtual int readPressedButton (); ///< returns the index of the pressed button or -1 if no button was pressed (->MessageBoxmanager->InteractiveMessageBox)

    virtual void onFrame (float frameDuration);

    /// \todo get rid of this stuff. Move it to the respective UI element classes, if needed.
    virtual std::map<int, MWMechanics::SkillValue > getPlayerSkillValues();
    virtual std::map<int, MWMechanics::AttributeValue > getPlayerAttributeValues();
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

    virtual bool getJournalAllowed() { return (mAllowed & GW_Magic) != 0; }

    virtual bool getPlayerSleeping();
    virtual void wakeUpPlayer();

    virtual void updatePlayer();

    virtual void showSoulgemDialog (MWWorld::Ptr item);

    virtual void changePointer (const std::string& name);

    virtual void setEnemy (const MWWorld::Ptr& enemy);

    virtual int getMessagesCount() const;

    virtual const Translation::Storage& getTranslationDataStorage() const;

    void onSoulgemDialogButtonPressed (int button);

    virtual bool getCursorVisible();

    /// Call when mouse cursor or buttons are used.
    virtual void setCursorActive(bool active);

    /// Clear all savegame-specific data
    virtual void clear();

    virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress);
    virtual void readRecord (ESM::ESMReader& reader, uint32_t type);
    virtual int countSavedGameRecords() const;

    /// Does the current stack of GUI-windows permit saving?
    virtual bool isSavingAllowed() const;

    /// Send exit command to active Modal window **/
    virtual void exitCurrentModal();

    /// Sets the current Modal
    /** Used to send exit command to active Modal when Esc is pressed **/
    virtual void addCurrentModal(WindowModal* input);

    /// Removes the top Modal
    /** Used when one Modal adds another Modal
        \param input Pointer to the current modal, to ensure proper modal is removed **/
    virtual void removeCurrentModal(WindowModal* input);

    virtual void pinWindow (MWGui::GuiWindow window);
    virtual void toggleMaximized(Layout *layout);

    /// Fade the screen in, over \a time seconds
    virtual void fadeScreenIn(const float time, bool clearQueue, float delay);
    /// Fade the screen out to black, over \a time seconds
    virtual void fadeScreenOut(const float time, bool clearQueue, float delay);
    /// Fade the screen to a specified percentage of black, over \a time seconds
    virtual void fadeScreenTo(const int percent, const float time, bool clearQueue, float delay);
    /// Darken the screen to a specified percentage
    virtual void setBlindness(const int percent);

    virtual void activateHitOverlay(bool interrupt);
    virtual void setWerewolfOverlay(bool set);

    virtual void toggleConsole();
    virtual void toggleDebugWindow();

    /// Cycle to next or previous spell
    virtual void cycleSpell(bool next);
    /// Cycle to next or previous weapon
    virtual void cycleWeapon(bool next);

    virtual void playSound(const std::string& soundId, float volume = 1.f, float pitch = 1.f);

    // In WindowManager for now since there isn't a VFS singleton
    virtual std::string correctIconPath(const std::string& path);
    virtual std::string correctBookartPath(const std::string& path, int width, int height, bool* exists = nullptr);
    virtual std::string correctTexturePath(const std::string& path);
    virtual bool textureExists(const std::string& path);

    void addCell(MWWorld::CellStore* cell);
    void removeCell(MWWorld::CellStore* cell);
    void writeFog(MWWorld::CellStore* cell);

    virtual const MWGui::TextColours& getTextColours();

    virtual bool injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat=false);
    virtual bool injectKeyRelease(MyGUI::KeyCode key);

  private:
    const MWWorld::ESMStore* mStore;
    Resource::ResourceSystem* mResourceSystem;
    osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

    osgMyGUI::Platform* mGuiPlatform;
    osgViewer::Viewer* mViewer;

    std::unique_ptr<Gui::FontLoader> mFontLoader;

    bool mConsoleOnlyScripts;

    std::map<MyGUI::Window*, std::string> mTrackedWindows;
    void trackWindow(Layout* layout, const std::string& name);
    void onWindowChangeCoord(MyGUI::Window* _sender);

    std::string mSelectedSpell;
    MWWorld::Ptr mSelectedEnchantItem;
    MWWorld::Ptr mSelectedWeapon;

    void loadFontDelegate(MyGUI::xml::ElementPtr _node, const std::string& _file, MyGUI::Version _version);

    std::vector<WindowModal*> mCurrentModals;

    // Markers placed manually by the player. Must be shared between both map views (the HUD map and the map window).
    CustomMarkerCollection mCustomMarkers;

    HUD *mHud;
    MapWindow *mMap;
    MWRender::LocalMap* mLocalMapRender;
    ToolTips *mToolTips;
    StatsWindow *mStatsWindow;
    MessageBoxManager *mMessageBoxManager;
    Console *mConsole;
    DialogueWindow *mDialogueWindow;
    DragAndDrop* mDragAndDrop;
    InventoryWindow *mInventoryWindow;
    ScrollWindow* mScrollWindow;
    BookWindow* mBookWindow;
    CountDialog* mCountDialog;
    TradeWindow* mTradeWindow;
    SettingsWindow* mSettingsWindow;
    ConfirmationDialog* mConfirmationDialog;
    SpellWindow* mSpellWindow;
    QuickKeysMenu* mQuickKeysMenu;
    LoadingScreen* mLoadingScreen;
    WaitDialog* mWaitDialog;
    SoulgemDialog* mSoulgemDialog;
    MyGUI::ImageBox* mVideoBackground;
    VideoWidget* mVideoWidget;
    ScreenFader* mWerewolfFader;
    ScreenFader* mBlindnessFader;
    ScreenFader* mHitFader;
    ScreenFader* mScreenFader;
    DebugWindow* mDebugWindow;
    JailScreen* mJailScreen;

    std::vector<WindowBase*> mWindows;

    Translation::Storage& mTranslationDataStorage;

    CharacterCreation* mCharGen;

    MyGUI::Widget* mInputBlocker;

    bool mCrosshairEnabled;
    bool mSubtitlesEnabled;
    bool mHitFaderEnabled;
    bool mWerewolfOverlayEnabled;
    bool mHudEnabled;
    bool mCursorVisible;
    bool mCursorActive;

    int mPlayerBounty;

    void setCursorVisible(bool visible);

    /// \todo get rid of this stuff. Move it to the respective UI element classes, if needed.
    // Various stats about player as needed by window manager
    std::string mPlayerName;
    std::string mPlayerRaceId;
    std::map<int, MWMechanics::AttributeValue > mPlayerAttributes;
    SkillList mPlayerMajorSkills, mPlayerMinorSkills;
    std::map<int, MWMechanics::SkillValue > mPlayerSkillValues;

    MyGUI::Gui *mGui; // Gui

    struct GuiModeState
    {
        GuiModeState(WindowBase* window)
        {
            mWindows.push_back(window);
        }
        GuiModeState(const std::vector<WindowBase*>& windows)
            : mWindows(windows) {}
        GuiModeState() {}

        void update(bool visible);

        std::vector<WindowBase*> mWindows;

        std::string mCloseSound;
        std::string mOpenSound;
    };
    // Defines the windows that should be shown in a particular GUI mode.
    std::map<GuiMode, GuiModeState> mGuiModeStates;
    // The currently active stack of GUI modes (top mode is the one we are in).
    std::vector<GuiMode> mGuiModes;

    SDLUtil::SDLCursorManager* mCursorManager;

    std::vector<Layout*> mGarbageDialogs;
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

    void updateMap();

    int mShowOwned;

    ToUTF8::FromType mEncoding;

    int mFontHeight;

    std::string mVersionDescription;

    MWGui::TextColours mTextColours;

    std::unique_ptr<KeyboardNavigation> mKeyboardNavigation;

    /**
     * Called when MyGUI tries to retrieve a tag's value. Tags must be denoted in #{tag} notation and will be replaced upon setting a user visible text/property.
     * Supported syntax:
     * #{GMSTName}: retrieves String value of the GMST called GMSTName
     * #{setting=CATEGORY_NAME,SETTING_NAME}: retrieves String value of SETTING_NAME under category CATEGORY_NAME from settings.cfg
     * #{sCell=CellID}: retrieves translated name of the given CellID (used only by some Morrowind localisations, in others cell ID is == cell name)
     * #{fontcolour=FontColourName}: retrieves the value of the fallback setting "FontColor_color_<FontColourName>" from openmw.cfg,
     *                              in the format "r g b a", float values in range 0-1. Useful for "Colour" and "TextColour" properties in skins.
     * #{fontcolourhtml=FontColourName}: retrieves the value of the fallback setting "FontColor_color_<FontColourName>" from openmw.cfg,
     *                              in the format "#xxxxxx" where x are hexadecimal numbers. Useful in an EditBox's caption to change the color of following text.
     */
    void onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

    void onCursorChange(const std::string& name);
    void onKeyFocusChanged(MyGUI::Widget* widget);

    // Key pressed while playing a video
    void onVideoKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char);

    void sizeVideo(int screenWidth, int screenHeight);

    void onClipboardChanged(const std::string& _type, const std::string& _data);
    void onClipboardRequested(const std::string& _type, std::string& _data);

    void createTextures();
    void createCursors();
    void setMenuTransparency(float value);

    void updatePinnedWindows();
  };
}

#endif
