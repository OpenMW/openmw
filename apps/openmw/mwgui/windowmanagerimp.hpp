#ifndef MWGUI_WINDOWMANAGERIMP_H
#define MWGUI_WINDOWMANAGERIMP_H

/**
   This class owns and controls all the MW specific windows in the
   GUI. It can enable/disable Gui mode, and is responsible for sending
   and retrieving information from the Gui.
**/

#include <memory>
#include <vector>

#include <osg/ref_ptr>

#include "../mwbase/windowmanager.hpp"
#include "../mwrender/localmap.hpp"

#include <components/misc/guarded.hpp>
#include <components/myguiplatform/myguiplatform.hpp>
#include <components/sdlutil/events.hpp>
#include <components/sdlutil/sdlcursormanager.hpp>
#include <components/sdlutil/sdlvideowrapper.hpp>
#include <components/settings/settings.hpp>
#include <components/toutf8/toutf8.hpp>

#include "charactercreation.hpp"
#include "draganddrop.hpp"
#include "mapwindow.hpp"
#include "messagebox.hpp"
#include "settings.hpp"
#include "soulgemdialog.hpp"
#include "statswatcher.hpp"
#include "textcolours.hpp"
#include "tooltips.hpp"
#include "windowbase.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_KeyCode.h>
#include <MyGUI_Types.h>
#include <filesystem>

namespace MyGUI
{
    class Widget;
    class Window;
    class UString;
    class ImageBox;
}

namespace MWWorld
{
    class Cell;
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

namespace Gui
{
    class FontLoader;
}

namespace MWGui
{
    class HUD;
    class MapWindow;
    class MainMenu;
    class StatsWindow;
    class InventoryWindow;
    struct JournalWindow;
    class TextInputDialog;
    class InfoBoxDialog;
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
    class Recharge;
    class CompanionWindow;
    class VideoWidget;
    class WindowModal;
    class ScreenFader;
    class DebugWindow;
    class PostProcessorHud;
    class JailScreen;
    class KeyboardNavigation;
    class ItemTransfer;
    class ControllerButtonsOverlay;
    class InventoryTabsOverlay;

    class WindowManager : public MWBase::WindowManager
    {
    public:
        typedef std::pair<std::string, int> Faction;
        typedef std::vector<Faction> FactionList;

        WindowManager(SDL_Window* window, osgViewer::Viewer* viewer, osg::Group* guiRoot,
            Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
            const std::filesystem::path& logpath, bool consoleOnlyScripts, Translation::Storage& translationDataStorage,
            ToUTF8::FromType encoding, bool exportFonts, const std::string& versionDescription, bool useShaders,
            Files::ConfigurationManager& cfgMgr);
        virtual ~WindowManager();

        /// Set the ESMStore to use for retrieving of GUI-related strings.
        void setStore(const MWWorld::ESMStore& store);

        void initUI();

        Loading::Listener* getLoadingScreen() override;

        /// @note This method will block until the video finishes playing
        /// (and will continually update the window while doing so)
        void playVideo(std::string_view name, bool allowSkipping, bool overrideSounds = true) override;

        /// Warning: do not use MyGUI::InputManager::setKeyFocusWidget directly. Instead use this.
        void setKeyFocusWidget(MyGUI::Widget* widget) override;

        void setNewGame(bool newgame) override;

        void pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg) override;
        void pushGuiMode(GuiMode mode) override;
        void popGuiMode(bool forceExit = false) override;
        void removeGuiMode(GuiMode mode) override; ///< can be anywhere in the stack

        void goToJail(int days) override;

        GuiMode getMode() const override;
        bool containsMode(GuiMode mode) const override;

        bool isGuiMode() const override;

        bool isConsoleMode() const override;
        bool isPostProcessorHudVisible() const override;
        bool isSettingsWindowVisible() const override;
        bool isInteractiveMessageBoxActive() const override;

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
        MWGui::HUD* getHud() override;
        MWGui::PostProcessorHud* getPostProcessorHud() override;
        std::vector<MWGui::WindowBase*> getGuiModeWindows(GuiMode mode) override;

        /// Make the player use an item, while updating GUI state accordingly
        void useItem(const MWWorld::Ptr& item, bool bypassBeastRestrictions = false) override;

        void updateSpellWindow() override;

        void setConsoleSelectedObject(const MWWorld::Ptr& object) override;
        MWWorld::Ptr getConsoleSelectedObject() const override;
        void printToConsole(const std::string& msg, std::string_view color) override;
        void setConsoleMode(std::string_view mode) override;
        const std::string& getConsoleMode() override;

        /// Set time left for the player to start drowning (update the drowning bar)
        /// @param time time left to start drowning
        /// @param maxTime how long we can be underwater (in total) until drowning starts
        void setDrowningTimeLeft(float time, float maxTime) override;

        void changeCell(const MWWorld::CellStore* cell) override; ///< change the active cell

        void setFocusObject(const MWWorld::Ptr& focus) override;
        void setFocusObjectScreenCoords(float x, float y) override;

        void getMousePosition(int& x, int& y) override;
        void getMousePosition(float& x, float& y) override;
        void setDragDrop(bool dragDrop) override;
        bool getWorldMouseOver() override;

        float getScalingFactor() const override;

        bool toggleFogOfWar() override;
        bool toggleFullHelp() override; ///< show extra info in item tooltips (owner, script)
        bool getFullHelp() const override;

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
        void activateQuickKey(int index) override;
        /// update activated quick key state (if action executing was delayed for some reason)
        void updateActivatedQuickKey() override;

        const ESM::RefId& getSelectedSpell() override { return mSelectedSpell; }
        void setSelectedSpell(const ESM::RefId& spellId, int successChancePercent) override;
        void setSelectedEnchantItem(const MWWorld::Ptr& item) override;
        const MWWorld::Ptr& getSelectedEnchantItem() const override;
        void setSelectedWeapon(const MWWorld::Ptr& item) override;
        const MWWorld::Ptr& getSelectedWeapon() const override;
        void unsetSelectedSpell() override;
        void unsetSelectedWeapon() override;

        void updateConsoleObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr) override;

        void showCrosshair(bool show) override;

        /// Turn visibility of HUD on or off
        bool setHudVisibility(bool show) override;
        bool isHudVisible() const override { return mHudEnabled; }

        void disallowMouse() override;
        void allowMouse() override;
        void notifyInputActionBound() override;

        void addVisitedLocation(const std::string& name, int x, int y) override;

        /// Hides dialog and schedules dialog to be deleted.
        void removeDialog(std::unique_ptr<Layout>&& dialog) override;

        /// Gracefully attempts to exit the topmost GUI mode
        void exitCurrentGuiMode() override;

        void messageBox(std::string_view message,
            enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible) override;
        void scheduleMessageBox(std::string message,
            enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible) override;
        void staticMessageBox(std::string_view message) override;
        void removeStaticMessageBox() override;
        void interactiveMessageBox(std::string_view message, const std::vector<std::string>& buttons = {},
            bool block = false, int defaultFocus = -1) override;

        int readPressedButton() override; ///< returns the index of the pressed button or -1 if no button was pressed
                                          ///< (->MessageBoxmanager->InteractiveMessageBox)

        void update(float duration);

        /**
         * Fetches a GMST string from the store, if there is no setting with the given
         * ID or it is not a string the default string is returned.
         *
         * @param id Identifier for the GMST setting, e.g. "aName"
         * @param default Default value if the GMST setting cannot be used.
         */
        std::string_view getGameSettingString(std::string_view id, std::string_view default_) override;

        void processChangedSettings(const Settings::CategorySettingVector& changed) override;

        void windowVisibilityChange(bool visible) override;
        void windowResized(int x, int y) override;
        void windowClosed() override;
        bool isWindowVisible() const override;

        void watchActor(const MWWorld::Ptr& ptr) override;
        MWWorld::Ptr getWatchedActor() const override;

        void executeInConsole(const std::filesystem::path& path) override;

        void enableRest() override { mRestAllowed = true; }
        bool getRestEnabled() override;

        bool getJournalAllowed() override { return (mAllowed & GW_Magic) != 0; }

        bool getPlayerSleeping() override;
        void wakeUpPlayer() override;

        void updatePlayer() override;

        void showSoulgemDialog(MWWorld::Ptr item) override;

        void changePointer(const std::string& name) override;

        void setEnemy(const MWWorld::Ptr& enemy) override;

        std::size_t getMessagesCount() const override;

        const Translation::Storage& getTranslationDataStorage() const override;

        bool getCursorVisible() override;

        /// Call when mouse cursor or buttons are used.
        void setCursorActive(bool active) override;

        /// Clear all savegame-specific data
        void clear() override;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress) override;
        void readRecord(ESM::ESMReader& reader, uint32_t type) override;
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

        void pinWindow(MWGui::GuiWindow window) override;
        void toggleMaximized(Layout* layout) override;

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
        void togglePostProcessorHud() override;
        void toggleSettingsWindow() override;

        /// Cycle to next or previous spell
        void cycleSpell(bool next) override;
        /// Cycle to next or previous weapon
        void cycleWeapon(bool next) override;

        void playSound(const ESM::RefId& soundId, float volume = 1.f, float pitch = 1.f) override;

        void addCell(MWWorld::CellStore* cell) override;
        void removeCell(MWWorld::CellStore* cell) override;
        void writeFog(MWWorld::CellStore* cell) override;

        const MWGui::TextColours& getTextColours() override;

        bool injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat = false) override;
        bool injectKeyRelease(MyGUI::KeyCode key) override;

        const std::string& getVersionDescription() const override;

        void onDeleteCustomData(const MWWorld::Ptr& ptr) override;
        void forceLootMode(const MWWorld::Ptr& ptr) override;

        void asyncPrepareSaveMap() override;

        WindowBase* getActiveControllerWindow() override;
        int getControllerMenuHeight() override;
        void cycleActiveControllerWindow(bool next) override;
        void setActiveControllerWindow(GuiMode mode, int activeIndex) override;
        bool getControllerTooltip() const override { return mControllerTooltip; }
        void setControllerTooltip(bool enabled) override;
        void updateControllerButtonsOverlay() override;

        // Used in Lua bindings
        const std::vector<GuiMode>& getGuiModeStack() const override { return mGuiModes; }
        void setDisabledByLua(std::string_view windowId, bool disabled) override;
        bool isWindowVisible(std::string_view windowId) const override;
        std::vector<std::string_view> getAllWindowIds() const override;
        std::vector<std::string_view> getAllowedWindowIds(GuiMode mode) const override;

    private:
        unsigned int mOldUpdateMask;
        unsigned int mOldCullMask;

        const MWWorld::ESMStore* mStore;
        Resource::ResourceSystem* mResourceSystem;
        osg::ref_ptr<SceneUtil::WorkQueue> mWorkQueue;

        std::unique_ptr<MyGUIPlatform::Platform> mGuiPlatform;
        osgViewer::Viewer* mViewer;

        std::unique_ptr<Gui::FontLoader> mFontLoader;
        std::unique_ptr<StatsWatcher> mStatsWatcher;

        bool mConsoleOnlyScripts;

        std::map<MyGUI::Window*, WindowSettingValues> mTrackedWindows;
        void trackWindow(Layout* layout, const WindowSettingValues& settings);
        void onWindowChangeCoord(MyGUI::Window* _sender);

        ESM::RefId mSelectedSpell;
        MWWorld::Ptr mSelectedEnchantItem;
        MWWorld::Ptr mSelectedWeapon;

        std::vector<WindowModal*> mCurrentModals;

        // Markers placed manually by the player. Must be shared between both map views (the HUD map and the map
        // window).
        CustomMarkerCollection mCustomMarkers;

        HUD* mHud;
        MapWindow* mMap;
        std::unique_ptr<MWRender::LocalMap> mLocalMapRender;
        std::unique_ptr<ToolTips> mToolTips;
        StatsWindow* mStatsWindow;
        std::unique_ptr<MessageBoxManager> mMessageBoxManager;
        Console* mConsole;
        DialogueWindow* mDialogueWindow;
        std::unique_ptr<DragAndDrop> mDragAndDrop;
        std::unique_ptr<ItemTransfer> mItemTransfer;
        InventoryWindow* mInventoryWindow;
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
        std::unique_ptr<SoulgemDialog> mSoulgemDialog;
        MyGUI::ImageBox* mVideoBackground;
        VideoWidget* mVideoWidget;
        ScreenFader* mWerewolfFader;
        ScreenFader* mBlindnessFader;
        ScreenFader* mHitFader;
        ScreenFader* mScreenFader;
        DebugWindow* mDebugWindow;
        PostProcessorHud* mPostProcessorHud;
        JailScreen* mJailScreen;
        ContainerWindow* mContainerWindow;
        ControllerButtonsOverlay* mControllerButtonsOverlay;
        InventoryTabsOverlay* mInventoryTabsOverlay;

        std::vector<std::unique_ptr<WindowBase>> mWindows;

        // Mapping windowId -> Window; used by Lua bindings.
        std::map<std::string_view, WindowBase*> mLuaIdToWindow;

        Translation::Storage& mTranslationDataStorage;

        std::unique_ptr<CharacterCreation> mCharGen;

        MyGUI::Widget* mInputBlocker;

        bool mHudEnabled;
        bool mCursorVisible;
        bool mCursorActive;

        int mPlayerBounty;

        void setCursorVisible(bool visible) override;

        std::unique_ptr<MyGUI::Gui> mGui; // Gui

        struct GuiModeState
        {
            GuiModeState(WindowBase* window) { mWindows.push_back(window); }
            GuiModeState(const std::vector<WindowBase*>& windows)
                : mWindows(windows)
            {
            }
            GuiModeState() {}

            void update(bool visible);

            std::vector<WindowBase*> mWindows;
        };
        // Defines the windows that should be shown in a particular GUI mode.
        std::map<GuiMode, GuiModeState> mGuiModeStates;
        // The currently active stack of GUI modes (top mode is the one we are in).
        std::vector<GuiMode> mGuiModes;
        // The active window for controller mode for each GUI mode.
        std::map<GuiMode, int> mActiveControllerWindows;
        bool mControllerTooltip;

        void reapplyActiveControllerWindow();

        std::unique_ptr<SDLUtil::SDLCursorManager> mCursorManager;

        std::vector<std::unique_ptr<Layout>> mGarbageDialogs;
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

        ToUTF8::FromType mEncoding;

        std::string mVersionDescription;

        bool mWindowVisible;

        MWGui::TextColours mTextColours;

        std::unique_ptr<KeyboardNavigation> mKeyboardNavigation;

        std::unique_ptr<SDLUtil::VideoWrapper> mVideoWrapper;

        float mScalingFactor;

        struct ScheduledMessageBox
        {
            std::string mMessage;
            MWGui::ShowInDialogueMode mShowInDialogueMode;

            ScheduledMessageBox(std::string&& message, MWGui::ShowInDialogueMode showInDialogueMode)
                : mMessage(std::move(message))
                , mShowInDialogueMode(showInDialogueMode)
            {
            }
        };

        Misc::ScopeGuarded<std::vector<ScheduledMessageBox>> mScheduledMessageBoxes;

        /**
         * Called when MyGUI tries to retrieve a tag's value. Tags must be denoted in #{tag} notation and will be
         * replaced upon setting a user visible text/property. Supported syntax:
         * #{GMSTName}: retrieves String value of the GMST called GMSTName
         * #{setting=CATEGORY_NAME,SETTING_NAME}: retrieves String value of SETTING_NAME under category CATEGORY_NAME
         * from settings.cfg
         * #{sCell=CellID}: retrieves translated name of the given CellID (used only by some Morrowind localisations, in
         * others cell ID is == cell name)
         * #{fontcolour=FontColourName}: retrieves the value of the fallback setting "FontColor_color_<FontColourName>"
         * from openmw.cfg, in the format "r g b a", float values in range 0-1. Useful for "Colour" and "TextColour"
         * properties in skins.
         * #{fontcolourhtml=FontColourName}: retrieves the value of the fallback setting
         * "FontColor_color_<FontColourName>" from openmw.cfg, in the format "#xxxxxx" where x are hexadecimal numbers.
         * Useful in an EditBox's caption to change the color of following text.
         */
        void onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

        void onCursorChange(std::string_view name);
        void onKeyFocusChanged(MyGUI::Widget* widget);

        // Key pressed while playing a video
        void onVideoKeyPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);

        void sizeVideo(int screenWidth, int screenHeight);

        void onClipboardChanged(std::string_view _type, std::string_view _data);
        void onClipboardRequested(std::string_view _type, std::string& _data);

        void createTextures();
        void createCursors();
        void setMenuTransparency(float value);

        void updatePinnedWindows();

        void enableScene(bool enable);

        void handleScheduledMessageBoxes();

        void pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg, bool force);

        void setCullMask(uint32_t mask) override;
        uint32_t getCullMask() override;

        void setActiveMap(const MWWorld::Cell& cell);
        ///< set the indices of the map texture that should be used

        Files::ConfigurationManager& mCfgMgr;
    };
}

#endif
