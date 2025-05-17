#ifndef GAME_MWBASE_WINDOWMANAGER_H
#define GAME_MWBASE_WINDOWMANAGER_H

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <MyGUI_KeyCode.h>

#include "../mwgui/mode.hpp"
#include "../mwgui/windowbase.hpp"

#include <components/sdlutil/events.hpp>

namespace ESM
{
    class RefId;
}

namespace Loading
{
    class Listener;
}

namespace Translation
{
    class Storage;
}

namespace MyGUI
{
    class Gui;
    class Widget;
    class UString;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWMechanics
{
    class AttributeValue;
    template <typename T>
    class DynamicStat;
    class SkillValue;
}

namespace MWWorld
{
    class CellStore;
    class Ptr;
}

namespace MWGui
{
    class Layout;

    class Console;
    class SpellWindow;
    class TradeWindow;
    class TravelWindow;
    class SpellBuyingWindow;
    class ConfirmationDialog;
    class CountDialog;
    class ScrollWindow;
    class BookWindow;
    class InventoryWindow;
    class ContainerWindow;
    class DialogueWindow;
    class WindowModal;
    class JailScreen;
    class MessageBox;
    class PostProcessorHud;
    class SettingsWindow;

    enum ShowInDialogueMode
    {
        ShowInDialogueMode_IfPossible,
        ShowInDialogueMode_Only,
        ShowInDialogueMode_Never
    };

    struct TextColours;
}

namespace SFO
{
    class CursorManager;
}

namespace MWBase
{
    /// \brief Interface for widnow manager (implemented in MWGui)
    class WindowManager : public SDLUtil::WindowListener
    {
        WindowManager(const WindowManager&);
        ///< not implemented

        WindowManager& operator=(const WindowManager&);
        ///< not implemented

    public:
        typedef std::vector<int> SkillList;

        WindowManager() {}

        virtual ~WindowManager() {}

        /// @note This method will block until the video finishes playing
        /// (and will continually update the window while doing so)
        virtual void playVideo(std::string_view name, bool allowSkipping, bool overrideSounds = true) = 0;

        virtual void setNewGame(bool newgame) = 0;

        virtual void pushGuiMode(MWGui::GuiMode mode, const MWWorld::Ptr& arg) = 0;
        virtual void pushGuiMode(MWGui::GuiMode mode) = 0;
        virtual void popGuiMode(bool forceExit = false) = 0;

        virtual void removeGuiMode(MWGui::GuiMode mode) = 0;
        ///< can be anywhere in the stack

        virtual void goToJail(int days) = 0;

        virtual void updatePlayer() = 0;

        virtual MWGui::GuiMode getMode() const = 0;
        virtual bool containsMode(MWGui::GuiMode) const = 0;

        virtual bool isGuiMode() const = 0;

        virtual bool isConsoleMode() const = 0;
        virtual bool isPostProcessorHudVisible() const = 0;
        virtual bool isSettingsWindowVisible() const = 0;
        virtual bool isInteractiveMessageBoxActive() const = 0;

        virtual void toggleVisible(MWGui::GuiWindow wnd) = 0;

        virtual void forceHide(MWGui::GuiWindow wnd) = 0;
        virtual void unsetForceHide(MWGui::GuiWindow wnd) = 0;

        /// Disallow all inventory mode windows
        virtual void disallowAll() = 0;

        /// Allow one or more windows
        virtual void allow(MWGui::GuiWindow wnd) = 0;

        virtual bool isAllowed(MWGui::GuiWindow wnd) const = 0;

        /// \todo investigate, if we really need to expose every single lousy UI element to the outside world
        virtual MWGui::InventoryWindow* getInventoryWindow() = 0;
        virtual MWGui::CountDialog* getCountDialog() = 0;
        virtual MWGui::ConfirmationDialog* getConfirmationDialog() = 0;
        virtual MWGui::TradeWindow* getTradeWindow() = 0;
        virtual MWGui::PostProcessorHud* getPostProcessorHud() = 0;

        /// Make the player use an item, while updating GUI state accordingly
        virtual void useItem(const MWWorld::Ptr& item, bool force = false) = 0;

        virtual void updateSpellWindow() = 0;

        virtual void setConsoleSelectedObject(const MWWorld::Ptr& object) = 0;
        virtual MWWorld::Ptr getConsoleSelectedObject() const = 0;
        virtual void setConsoleMode(std::string_view mode) = 0;
        virtual const std::string& getConsoleMode() = 0;

        static constexpr std::string_view sConsoleColor_Default = "#FFFFFF";
        static constexpr std::string_view sConsoleColor_Error = "#FF2222";
        static constexpr std::string_view sConsoleColor_Success = "#FF00FF";
        static constexpr std::string_view sConsoleColor_Info = "#AAAAAA";
        virtual void printToConsole(const std::string& msg, std::string_view color) = 0;

        /// Set time left for the player to start drowning (update the drowning bar)
        /// @param time time left to start drowning
        /// @param maxTime how long we can be underwater (in total) until drowning starts
        virtual void setDrowningTimeLeft(float time, float maxTime) = 0;

        virtual void changeCell(const MWWorld::CellStore* cell) = 0;
        ///< change the active cell

        virtual void setFocusObject(const MWWorld::Ptr& focus) = 0;
        virtual void setFocusObjectScreenCoords(float x, float y) = 0;

        virtual void setCursorVisible(bool visible) = 0;
        virtual void setCursorActive(bool active) = 0;
        virtual void getMousePosition(int& x, int& y) = 0;
        virtual void getMousePosition(float& x, float& y) = 0;
        virtual void setDragDrop(bool dragDrop) = 0;
        virtual bool getWorldMouseOver() = 0;

        virtual float getScalingFactor() const = 0;

        virtual bool toggleFogOfWar() = 0;

        virtual bool toggleFullHelp() = 0;
        ///< show extra info in item tooltips (owner, script)

        virtual bool getFullHelp() const = 0;

        /// sets the visibility of the drowning bar
        virtual void setDrowningBarVisibility(bool visible) = 0;

        /// sets the visibility of the hud health/magicka/stamina bars
        virtual void setHMSVisibility(bool visible) = 0;

        /// sets the visibility of the hud minimap
        virtual void setMinimapVisibility(bool visible) = 0;
        virtual void setWeaponVisibility(bool visible) = 0;
        virtual void setSpellVisibility(bool visible) = 0;
        virtual void setSneakVisibility(bool visible) = 0;

        /// activate selected quick key
        virtual void activateQuickKey(int index) = 0;
        /// update activated quick key state (if action executing was delayed for some reason)
        virtual void updateActivatedQuickKey() = 0;

        virtual const ESM::RefId& getSelectedSpell() = 0;
        virtual void setSelectedSpell(const ESM::RefId& spellId, int successChancePercent) = 0;
        virtual void setSelectedEnchantItem(const MWWorld::Ptr& item) = 0;
        virtual const MWWorld::Ptr& getSelectedEnchantItem() const = 0;
        virtual void setSelectedWeapon(const MWWorld::Ptr& item) = 0;
        virtual const MWWorld::Ptr& getSelectedWeapon() const = 0;
        virtual void unsetSelectedSpell() = 0;
        virtual void unsetSelectedWeapon() = 0;

        virtual void showCrosshair(bool show) = 0;
        virtual bool setHudVisibility(bool show) = 0;
        virtual bool isHudVisible() const = 0;

        virtual void disallowMouse() = 0;
        virtual void allowMouse() = 0;
        virtual void notifyInputActionBound() = 0;

        virtual void addVisitedLocation(const std::string& name, int x, int y) = 0;

        /// Hides dialog and schedules dialog to be deleted.
        virtual void removeDialog(std::unique_ptr<MWGui::Layout>&& dialog) = 0;

        /// Gracefully attempts to exit the topmost GUI mode
        /** No guarantee of actually closing the window **/
        virtual void exitCurrentGuiMode() = 0;

        virtual void messageBox(std::string_view message,
            enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible)
            = 0;
        /// Puts message into a queue to show on the next update. Thread safe alternative for messageBox.
        virtual void scheduleMessageBox(std::string message,
            enum MWGui::ShowInDialogueMode showInDialogueMode = MWGui::ShowInDialogueMode_IfPossible)
            = 0;
        virtual void staticMessageBox(std::string_view message) = 0;
        virtual void removeStaticMessageBox() = 0;
        virtual void interactiveMessageBox(std::string_view message, const std::vector<std::string>& buttons = {},
            bool block = false, int defaultFocus = -1)
            = 0;

        /// returns the index of the pressed button or -1 if no button was pressed
        /// (->MessageBoxmanager->InteractiveMessageBox)
        virtual int readPressedButton() = 0;

        virtual void updateConsoleObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr) = 0;

        /**
         * Fetches a GMST string from the store, if there is no setting with the given
         * ID or it is not a string the default string is returned.
         *
         * @param id Identifier for the GMST setting, e.g. "aName"
         * @param default Default value if the GMST setting cannot be used.
         */
        virtual std::string_view getGameSettingString(std::string_view id, std::string_view default_) = 0;

        virtual void processChangedSettings(const std::set<std::pair<std::string, std::string>>& changed) = 0;

        virtual void executeInConsole(const std::filesystem::path& path) = 0;

        virtual void enableRest() = 0;
        virtual bool getRestEnabled() = 0;
        virtual bool getJournalAllowed() = 0;

        virtual bool getPlayerSleeping() = 0;
        virtual void wakeUpPlayer() = 0;

        virtual void showSoulgemDialog(MWWorld::Ptr item) = 0;

        virtual void changePointer(const std::string& name) = 0;

        virtual void setEnemy(const MWWorld::Ptr& enemy) = 0;

        virtual std::size_t getMessagesCount() const = 0;

        virtual const Translation::Storage& getTranslationDataStorage() const = 0;

        /// Warning: do not use MyGUI::InputManager::setKeyFocusWidget directly. Instead use this.
        virtual void setKeyFocusWidget(MyGUI::Widget* widget) = 0;

        virtual Loading::Listener* getLoadingScreen() = 0;

        /// Should the cursor be visible?
        virtual bool getCursorVisible() = 0;

        /// Clear all savegame-specific data
        virtual void clear() = 0;

        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;
        virtual int countSavedGameRecords() const = 0;

        /// Does the current stack of GUI-windows permit saving?
        virtual bool isSavingAllowed() const = 0;

        /// Send exit command to active Modal window
        virtual void exitCurrentModal() = 0;

        /// Sets the current Modal
        /** Used to send exit command to active Modal when Esc is pressed **/
        virtual void addCurrentModal(MWGui::WindowModal* input) = 0;

        /// Removes the top Modal
        /** Used when one Modal adds another Modal
            \param input Pointer to the current modal, to ensure proper modal is removed **/
        virtual void removeCurrentModal(MWGui::WindowModal* input) = 0;

        virtual void pinWindow(MWGui::GuiWindow window) = 0;
        virtual void toggleMaximized(MWGui::Layout* layout) = 0;

        /// Fade the screen in, over \a time seconds
        virtual void fadeScreenIn(const float time, bool clearQueue = true, float delay = 0.f) = 0;
        /// Fade the screen out to black, over \a time seconds
        virtual void fadeScreenOut(const float time, bool clearQueue = true, float delay = 0.f) = 0;
        /// Fade the screen to a specified percentage of black, over \a time seconds
        virtual void fadeScreenTo(const int percent, const float time, bool clearQueue = true, float delay = 0.f) = 0;
        /// Darken the screen to a specified percentage
        virtual void setBlindness(const int percent) = 0;

        virtual void activateHitOverlay(bool interrupt = true) = 0;
        virtual void setWerewolfOverlay(bool set) = 0;

        virtual void toggleConsole() = 0;
        virtual void toggleDebugWindow() = 0;
        virtual void togglePostProcessorHud() = 0;
        virtual void toggleSettingsWindow() = 0;

        /// Cycle to next or previous spell
        virtual void cycleSpell(bool next) = 0;
        /// Cycle to next or previous weapon
        virtual void cycleWeapon(bool next) = 0;

        virtual void playSound(const ESM::RefId& soundId, float volume = 1.f, float pitch = 1.f) = 0;

        virtual void addCell(MWWorld::CellStore* cell) = 0;
        virtual void removeCell(MWWorld::CellStore* cell) = 0;
        virtual void writeFog(MWWorld::CellStore* cell) = 0;

        virtual const MWGui::TextColours& getTextColours() = 0;

        virtual bool injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat) = 0;
        virtual bool injectKeyRelease(MyGUI::KeyCode key) = 0;

        void windowVisibilityChange(bool visible) override = 0;
        void windowResized(int x, int y) override = 0;
        void windowClosed() override = 0;
        virtual bool isWindowVisible() const = 0;

        virtual void watchActor(const MWWorld::Ptr& ptr) = 0;
        virtual MWWorld::Ptr getWatchedActor() const = 0;

        virtual const std::string& getVersionDescription() const = 0;

        virtual void onDeleteCustomData(const MWWorld::Ptr& ptr) = 0;
        virtual void forceLootMode(const MWWorld::Ptr& ptr) = 0;

        virtual void asyncPrepareSaveMap() = 0;

        /// Sets the cull masks for all applicable views
        virtual void setCullMask(uint32_t mask) = 0;

        /// Same as viewer->getCamera()->getCullMask(), provided for consistency.
        virtual uint32_t getCullMask() = 0;

        /// Return the window that should receive controller events
        virtual MWGui::WindowBase* getActiveControllerWindow() = 0;
        /// Cycle to the next window to receive controller events
        virtual void cycleActiveControllerWindow(bool next) = 0;
        virtual void updateControllerButtonsOverlay() = 0;

        // Used in Lua bindings
        virtual const std::vector<MWGui::GuiMode>& getGuiModeStack() const = 0;
        virtual void setDisabledByLua(std::string_view windowId, bool disabled) = 0;
        virtual std::vector<std::string_view> getAllWindowIds() const = 0;
        virtual std::vector<std::string_view> getAllowedWindowIds(MWGui::GuiMode mode) const = 0;
    };
}

#endif
