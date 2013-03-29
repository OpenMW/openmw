#ifndef GAME_MWBASE_WINDOWMANAGER_H
#define GAME_MWBASE_WINDOWMANAGER_H

#include <string>
#include <vector>
#include <map>

#include <components/settings/settings.hpp>

#include <components/translation/translation.hpp>

#include "../mwmechanics/stat.hpp"

#include "../mwgui/mode.hpp"

namespace MyGUI
{
    class Gui;
    class Widget;
    class UString;
}

namespace OEngine
{
    namespace GUI
    {
        class Layout;
    }
}

namespace ESM
{
    struct Class;
}

namespace MWWorld
{
    class CellStore;
    class Ptr;
}

namespace MWGui
{
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
}

namespace MWBase
{
    /// \brief Interface for widnow manager (implemented in MWGui)
    class WindowManager
    {
            WindowManager (const WindowManager&);
            ///< not implemented

            WindowManager& operator= (const WindowManager&);
            ///< not implemented

        public:

            typedef std::vector<int> SkillList;

            WindowManager() {}

            virtual ~WindowManager() {}

            /**
             * Should be called each frame to update windows/gui elements.
             * This could mean updating sizes of gui elements or opening
             * new dialogs.
             */
            virtual void update() = 0;

            virtual void pushGuiMode (MWGui::GuiMode mode) = 0;
            virtual void popGuiMode() = 0;

            virtual void removeGuiMode (MWGui::GuiMode mode) = 0;
            ///< can be anywhere in the stack

            virtual MWGui::GuiMode getMode() const = 0;

            virtual bool isGuiMode() const = 0;

            virtual bool isConsoleMode() const = 0;

            virtual void toggleVisible (MWGui::GuiWindow wnd) = 0;

            /// Disallow all inventory mode windows
            virtual void disallowAll() = 0;

            /// Allow one or more windows
            virtual void allow (MWGui::GuiWindow wnd) = 0;

            virtual bool isAllowed (MWGui::GuiWindow wnd) const = 0;

            /// \todo investigate, if we really need to expose every single lousy UI element to the outside world
            virtual MWGui::DialogueWindow* getDialogueWindow() = 0;
            virtual MWGui::ContainerWindow* getContainerWindow() = 0;
            virtual MWGui::InventoryWindow* getInventoryWindow() = 0;
            virtual MWGui::BookWindow* getBookWindow() = 0;
            virtual MWGui::ScrollWindow* getScrollWindow() = 0;
            virtual MWGui::CountDialog* getCountDialog() = 0;
            virtual MWGui::ConfirmationDialog* getConfirmationDialog() = 0;
            virtual MWGui::TradeWindow* getTradeWindow() = 0;
            virtual MWGui::SpellBuyingWindow* getSpellBuyingWindow() = 0;
            virtual MWGui::TravelWindow* getTravelWindow() = 0;
            virtual MWGui::SpellWindow* getSpellWindow() = 0;
            virtual MWGui::Console* getConsole() = 0;

            virtual MyGUI::Gui* getGui() const = 0;

            virtual void wmUpdateFps(float fps, unsigned int triangleCount, unsigned int batchCount) = 0;

            /// Set value for the given ID.
            virtual void setValue (const std::string& id, const MWMechanics::Stat<int>& value) = 0;
            virtual void setValue (int parSkill, const MWMechanics::Stat<float>& value) = 0;
            virtual void setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value) = 0;
            virtual void setValue (const std::string& id, const std::string& value) = 0;
            virtual void setValue (const std::string& id, int value) = 0;

            virtual void setPlayerClass (const ESM::Class &class_) = 0;
            ///< set current class of player

            virtual void configureSkills (const SkillList& major, const SkillList& minor) = 0;
            ///< configure skill groups, each set contains the skill ID for that group.

            virtual void setReputation (int reputation) = 0;
            ///< set the current reputation value

            virtual void setBounty (int bounty) = 0;
            ///< set the current bounty value

            virtual void updateSkillArea() = 0;
            ///< update display of skills, factions, birth sign, reputation and bounty

            virtual void changeCell(MWWorld::CellStore* cell) = 0;
            ///< change the active cell

            virtual void setPlayerPos(const float x, const float y) = 0;
            ///< set player position in map space

            virtual void setPlayerDir(const float x, const float y) = 0;
            ///< set player view direction in map space

            virtual void setFocusObject(const MWWorld::Ptr& focus) = 0;
            virtual void setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y) = 0;

            virtual void setMouseVisible(bool visible) = 0;
            virtual void getMousePosition(int &x, int &y) = 0;
            virtual void getMousePosition(float &x, float &y) = 0;
            virtual void setDragDrop(bool dragDrop) = 0;
            virtual bool getWorldMouseOver() = 0;

            virtual void toggleFogOfWar() = 0;

            virtual void toggleFullHelp() = 0;
            ///< show extra info in item tooltips (owner, script)

            virtual bool getFullHelp() const = 0;

            virtual void setInteriorMapTexture(const int x, const int y) = 0;
            ///< set the index of the map texture that should be used (for interiors)

            /// sets the visibility of the hud health/magicka/stamina bars
            virtual void setHMSVisibility(bool visible) = 0;

            /// sets the visibility of the hud minimap
            virtual void setMinimapVisibility(bool visible) = 0;
            virtual void setWeaponVisibility(bool visible) = 0;
            virtual void setSpellVisibility(bool visible) = 0;

            virtual void activateQuickKey  (int index) = 0;

            virtual void setSelectedSpell(const std::string& spellId, int successChancePercent) = 0;
            virtual void setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent) = 0;
            virtual void setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent) = 0;
            virtual void unsetSelectedSpell() = 0;
            virtual void unsetSelectedWeapon() = 0;

            virtual void showCrosshair(bool show) = 0;
            virtual bool getSubtitlesEnabled() = 0;
            virtual void toggleHud() = 0;

            virtual void disallowMouse() = 0;
            virtual void allowMouse() = 0;
            virtual void notifyInputActionBound() = 0;

            virtual void addVisitedLocation(const std::string& name, int x, int y) = 0;

            virtual void removeDialog(OEngine::GUI::Layout* dialog) = 0;
            ///< Hides dialog and schedules dialog to be deleted.

            virtual void messageBox (const std::string& message, const std::vector<std::string>& buttons = std::vector<std::string>()) = 0;
            virtual void enterPressed () = 0;
            virtual int readPressedButton() = 0;
            ///< returns the index of the pressed button or -1 if no button was pressed (->MessageBoxmanager->InteractiveMessageBox)

            virtual void onFrame (float frameDuration) = 0;

            /// \todo get rid of this stuff. Move it to the respective UI element classes, if needed.
            virtual std::map<int, MWMechanics::Stat<float> > getPlayerSkillValues() = 0;
            virtual std::map<int, MWMechanics::Stat<int> > getPlayerAttributeValues() = 0;
            virtual SkillList getPlayerMinorSkills() = 0;
            virtual SkillList getPlayerMajorSkills() = 0;

            /**
             * Fetches a GMST string from the store, if there is no setting with the given
             * ID or it is not a string the default string is returned.
             *
             * @param id Identifier for the GMST setting, e.g. "aName"
             * @param default Default value if the GMST setting cannot be used.
             */
            virtual std::string getGameSettingString(const std::string &id, const std::string &default_) = 0;

            virtual void processChangedSettings(const Settings::CategorySettingVector& changed) = 0;

            virtual void executeInConsole (const std::string& path) = 0;

            virtual void setLoadingProgress (const std::string& stage, int depth, int current, int total) = 0;
            virtual void loadingDone() = 0;

            virtual void enableRest() = 0;
            virtual bool getRestEnabled() = 0;

            virtual bool getPlayerSleeping() = 0;
            virtual void wakeUpPlayer() = 0;

            virtual void startSpellMaking(MWWorld::Ptr actor) = 0;
            virtual void startEnchanting(MWWorld::Ptr actor) = 0;
            virtual void startTraining(MWWorld::Ptr actor) = 0;
            virtual void startRepair(MWWorld::Ptr actor) = 0;
            virtual void startRepairItem(MWWorld::Ptr item) = 0;

            virtual void changePointer (const std::string& name) = 0;

            virtual const Translation::Storage& getTranslationDataStorage() const = 0;
    };
}

#endif
