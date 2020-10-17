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

#include <components/sdlutil/events.hpp>
#include <components/settings/settings.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include "mapwindow.hpp"
#include "statswatcher.hpp"
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
    class VideoWrapper;
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

  class WindowManager :
      public MWBase::WindowManager
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;

    WindowManager(SDL_Window* window, osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
                  const std::string& logpath, const std::string& cacheDir, bool consoleOnlyScripts, Translation::Storage& translationDataStorage,
                  ToUTF8::FromType encoding, bool exportFonts, const std::string& versionDescription, const std::string& localPath);
    virtual ~WindowManager();

    /// Set the ESMStore to use for retrieving of GUI-related strings.
    void setStore (const MWWorld::ESMStore& store);

    void initUI();
    void loadUserFonts() override;

    Loading::Listener* getLoadingScreen() override;

    /// @note This method will block until the video finishes playing
    /// (and will continually update the window while doing so)
    void playVideo(const std::string& name, bool allowSkipping) override;

    /// Warning: do not use MyGUI::InputManager::setKeyFocusWidget directly. Instead use this.
    void setKeyFocusWidget (MyGUI::Widget* widget) override;

    void setNewGame(bool newgame) override;

    void pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg) override;
    void pushGuiMode (GuiMode mode) override;
    void popGuiMode(bool noSound=false) override;
    void removeGuiMode(GuiMode mode, bool noSound=false) override; ///< can be anywhere in the stack

    void goToJail(int days) override;

    GuiMode getMode() const override;
    bool containsMode(GuiMode mode) const override;

    bool isGuiMode() const override;

    bool isConsoleMode() const override;

    void toggleVisible(GuiWindow wnd) override;

    void forceHide(MWGui::GuiWindow wnd) override;
    void unsetForceHide(MWGui::GuiWindow wnd) override;

    /// Disallow all inventory mode windows
    void disallowAll() override;

    /// Allow one or more windows
    void allow(GuiWindow wnd) override;

    bool isAllowed(GuiWindow wnd) const override;

    /// \todo investigate, if we really need to expose every single lousy UI element to the outside world
    MWGui::InventoryWindow* getInventoryWindow() override;
    MWGui::CountDialog* getCountDialog() override;
    MWGui::ConfirmationDialog* getConfirmationDialog() override;
    MWGui::TradeWindow* getTradeWindow() override;

    /// Make the player use an item, while updating GUI state accordingly
    void useItem(const MWWorld::Ptr& item, bool bypassBeastRestrictions=false) override;

    void updateSpellWindow() override;

    void setConsoleSelectedObject(const MWWorld::Ptr& object) override;

    /// Set time left for the player to start drowning (update the drowning bar)
    /// @param time time left to start drowning
    /// @param maxTime how long we can be underwater (in total) until drowning starts
    void setDrowningTimeLeft (float time, float maxTime) override;

    void changeCell(const MWWorld::CellStore* cell) override; ///< change the active cell

    void setFocusObject(const MWWorld::Ptr& focus) override;
    void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y) override;

    void getMousePosition(int &x, int &y) override;
    void getMousePosition(float &x, float &y) override;
    void setDragDrop(bool dragDrop) override;
    bool getWorldMouseOver() override;

    bool toggleFogOfWar() override;
    bool toggleFullHelp() override; ///< show extra info in item tooltips (owner, script)
    bool getFullHelp() const override;

    void setActiveMap(int x, int y, bool interior) override;
    ///< set the indices of the map texture that should be used

    /// sets the visibility of the drowning bar
    void setDrowningBarVisibility(bool visible) override;

    // sets the visibility of the hud health/magicka/stamina bars
    void setHMSVisibility(bool visible) override;
    // sets the visibility of the hud minimap
    void setMinimapVisibility(bool visible) override;
    void setWeaponVisibility(bool visible) override;
    void setSpellVisibility(bool visible) override;
    void setSneakVisibility(bool visible) override;

    /// activate selected quick key
    void activateQuickKey (int index) override;
    /// update activated quick key state (if action executing was delayed for some reason)
    void updateActivatedQuickKey () override;

    std::string getSelectedSpell() override { return mSelectedSpell; }
    void setSelectedSpell(const std::string& spellId, int successChancePercent) override;
    void setSelectedEnchantItem(const MWWorld::Ptr& item) override;
    const MWWorld::Ptr& getSelectedEnchantItem() const override;
    void setSelectedWeapon(const MWWorld::Ptr& item) override;
    const MWWorld::Ptr& getSelectedWeapon() const override;
    int getFontHeight() const override;
    void unsetSelectedSpell() override;
    void unsetSelectedWeapon() override;

    void updateConsoleObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr) override;

    void showCrosshair(bool show) override;
    bool getSubtitlesEnabled() override;

    /// Turn visibility of HUD on or off
    bool toggleHud() override;

    void disallowMouse() override;
    void allowMouse() override;
    void notifyInputActionBound() override;

    void addVisitedLocation(const std::string& name, int x, int y) override;

    ///Hides dialog and schedules dialog to be deleted.
    void removeDialog(Layout* dialog) override;

    ///Gracefully attempts to exit the topmost GUI mode
    void exitCurrentGuiMode() override;

    void messageBox (const std::string& message, enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible) override;
    void staticMessageBox(const std::string& message) override;
    void removeStaticMessageBox() override;
    void interactiveMessageBox (const std::string& message,
                                        const std::vector<std::string>& buttons = std::vector<std::string>(), bool block=false) override;

    int readPressedButton () override; ///< returns the index of the pressed button or -1 if no button was pressed (->MessageBoxmanager->InteractiveMessageBox)

    void update (float duration) override;

    /**
     * Fetches a GMST string from the store, if there is no setting with the given
     * ID or it is not a string the default string is returned.
     *
     * @param id Identifier for the GMST setting, e.g. "aName"
     * @param default Default value if the GMST setting cannot be used.
     */
    std::string getGameSettingString(const std::string &id, const std::string &default_) override;

    void processChangedSettings(const Settings::CategorySettingVector& changed) override;

    void windowVisibilityChange(bool visible) override;
    void windowResized(int x, int y) override;
    void windowClosed() override;
    bool isWindowVisible() override;

    void watchActor(const MWWorld::Ptr& ptr) override;
    MWWorld::Ptr getWatchedActor() const override;

    void executeInConsole (const std::string& path) override;

    void enableRest() override { mRestAllowed = true; }
    bool getRestEnabled() override;

    bool getJournalAllowed() override { return (mAllowed & GW_Magic) != 0; }

    bool getPlayerSleeping() override;
    void wakeUpPlayer() override;

    void updatePlayer() override;

    void showSoulgemDialog (MWWorld::Ptr item) override;

    void changePointer (const std::string& name) override;

    void setEnemy (const MWWorld::Ptr& enemy) override;

    int getMessagesCount() const override;

    const Translation::Storage& getTranslationDataStorage() const override;

    void onSoulgemDialogButtonPressed (int button);

    bool getCursorVisible() override;

    /// Call when mouse cursor or buttons are used.
    void setCursorActive(bool active) override;

    /// Clear all savegame-specific data
    void clear() override;

    void write (ESM::ESMWriter& writer, Loading::Listener& progress) override;
    void readRecord (ESM::ESMReader& reader, uint32_t type) override;
    int countSavedGameRecords() const override;

    /// Does the current stack of GUI-windows permit saving?
    bool isSavingAllowed() const override;

    /// Send exit command to active Modal window **/
    void exitCurrentModal() override;

    /// Sets the current Modal
    /** Used to send exit command to active Modal when Esc is pressed **/
    void addCurrentModal(WindowModal* input) override;

    /// Removes the top Modal
    /** Used when one Modal adds another Modal
        \param input Pointer to the current modal, to ensure proper modal is removed **/
    void removeCurrentModal(WindowModal* input) override;

    void pinWindow (MWGui::GuiWindow window) override;
    void toggleMaximized(Layout *layout) override;

    /// Fade the screen in, over \a time seconds
    void fadeScreenIn(const float time, bool clearQueue, float delay) override;
    /// Fade the screen out to black, over \a time seconds
    void fadeScreenOut(const float time, bool clearQueue, float delay) override;
    /// Fade the screen to a specified percentage of black, over \a time seconds
    void fadeScreenTo(const int percent, const float time, bool clearQueue, float delay) override;
    /// Darken the screen to a specified percentage
    void setBlindness(const int percent) override;

    void activateHitOverlay(bool interrupt) override;
    void setWerewolfOverlay(bool set) override;

    void toggleConsole() override;
    void toggleDebugWindow() override;

    /// Cycle to next or previous spell
    void cycleSpell(bool next) override;
    /// Cycle to next or previous weapon
    void cycleWeapon(bool next) override;

    void playSound(const std::string& soundId, float volume = 1.f, float pitch = 1.f) override;

    // In WindowManager for now since there isn't a VFS singleton
    std::string correctIconPath(const std::string& path) override;
    std::string correctBookartPath(const std::string& path, int width, int height, bool* exists = nullptr) override;
    std::string correctTexturePath(const std::string& path) override;
    bool textureExists(const std::string& path) override;

    void addCell(MWWorld::CellStore* cell) override;
    void removeCell(MWWorld::CellStore* cell) override;
    void writeFog(MWWorld::CellStore* cell) override;

    const MWGui::TextColours& getTextColours() override;

    bool injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat=false) override;
    bool injectKeyRelease(MyGUI::KeyCode key) override;

  private:
    unsigned int mOldUpdateMask; unsigned int mOldCullMask;

    const MWWorld::ESMStore* mStore;
    Resource::ResourceSystem* mResourceSystem;
    osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

    osgMyGUI::Platform* mGuiPlatform;
    osgViewer::Viewer* mViewer;

    std::unique_ptr<Gui::FontLoader> mFontLoader;
    std::unique_ptr<StatsWatcher> mStatsWatcher;

    bool mConsoleOnlyScripts;

    std::map<MyGUI::Window*, std::string> mTrackedWindows;
    void trackWindow(Layout* layout, const std::string& name);
    void onWindowChangeCoord(MyGUI::Window* _sender);

    std::string mSelectedSpell;
    MWWorld::Ptr mSelectedEnchantItem;
    MWWorld::Ptr mSelectedWeapon;

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

    void setCursorVisible(bool visible) override;

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

    std::string mVersionDescription;

    bool mWindowVisible;

    MWGui::TextColours mTextColours;

    std::unique_ptr<KeyboardNavigation> mKeyboardNavigation;

    SDLUtil::VideoWrapper* mVideoWrapper;

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

    void enableScene(bool enable);
  };
}

#endif
