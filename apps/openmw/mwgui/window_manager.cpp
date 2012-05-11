#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "map_window.hpp"
#include "stats_window.hpp"
#include "messagebox.hpp"
#include "container.hpp"
#include "inventorywindow.hpp"
#include "itemwidget.hpp"
#include "tooltips.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwinput/inputmanager.hpp"

#include "../mwbase/environment.hpp"

#include "console.hpp"
#include "journalwindow.hpp"
#include "charactercreation.hpp"

#include <components/settings/settings.hpp>

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

WindowManager::WindowManager(
    const Compiler::Extensions& extensions, int fpsLevel, bool newGame, OEngine::Render::OgreRenderer *mOgre, const std::string logpath)
  : mGuiManager(NULL)
  , hud(NULL)
  , map(NULL)
  , menu(NULL)
  , stats(NULL)
  , mToolTips(NULL)
  , mMessageBoxManager(NULL)
  , console(NULL)
  , mJournal(NULL)
  , mDialogueWindow(nullptr)
  , mCharGen(NULL)
  , playerClass()
  , playerName()
  , playerRaceId()
  , playerBirthSignId()
  , playerAttributes()
  , playerMajorSkills()
  , playerMinorSkills()
  , playerSkillValues()
  , playerHealth()
  , playerMagicka()
  , playerFatigue()
  , gui(NULL)
  , mode(GM_Game)
  , nextMode(GM_Game)
  , needModeChange(false)
  , garbageDialogs()
  , shown(GW_ALL)
  , allowed(newGame ? GW_None : GW_ALL)
  , showFPSLevel(fpsLevel)
  , mFPS(0.0f)
  , mTriangleCount(0)
  , mBatchCount(0)
{

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre->getWindow(), mOgre->getScene(), false, logpath);
    gui = mGuiManager->getGui();
    
    //Register own widgets with MyGUI
    MyGUI::FactoryManager::getInstance().registerFactory<DialogueHistory>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<ItemWidget>("Widget");

    // Get size info from the Gui object
    assert(gui);
    int w = MyGUI::RenderManager::getInstance().getViewSize().width;
    int h = MyGUI::RenderManager::getInstance().getViewSize().height;

    MyGUI::Widget* dragAndDropWidget = gui->createWidgetT("Widget","",0,0,w,h,MyGUI::Align::Default,"DragAndDrop","DragAndDropWidget");
    dragAndDropWidget->setVisible(false);

    DragAndDrop* mDragAndDrop = new DragAndDrop();
    mDragAndDrop->mIsOnDragAndDrop = false;
    mDragAndDrop->mDraggedWidget = 0;
    mDragAndDrop->mDragAndDropWidget = dragAndDropWidget;
    mDragAndDrop->mContainerWindow = 0;

    hud = new HUD(w,h, showFPSLevel);
    menu = new MainMenu(w,h);
    map = new MapWindow(*this);
    stats = new StatsWindow(*this);
    console = new Console(w,h, extensions);
    mJournal = new JournalWindow(*this);
    mMessageBoxManager = new MessageBoxManager(this);
    mDialogueWindow = new DialogueWindow(*this);
    mContainerWindow = new ContainerWindow(*this,mDragAndDrop);
    mInventoryWindow = new InventoryWindow(*this,mDragAndDrop);
    mToolTips = new ToolTips(this);

    // The HUD is always on
    hud->setVisible(true);

    mCharGen = new CharacterCreation(this);

    // Setup player stats
    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        playerAttributes.insert(std::make_pair(ESM::Attribute::attributeIds[i], MWMechanics::Stat<int>()));
    }

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        playerSkillValues.insert(std::make_pair(ESM::Skill::skillIds[i], MWMechanics::Stat<float>()));
    }

    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWEffectList>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");

    // Set up visibility
    updateVisible();
}

WindowManager::~WindowManager()
{
    delete mGuiManager;
    delete console;
    delete mMessageBoxManager;
    delete hud;
    delete map;
    delete menu;
    delete stats;
    delete mJournal;
    delete mDialogueWindow;
    delete mContainerWindow;
    delete mInventoryWindow;
    delete mToolTips;
    delete mCharGen;
    delete mDragAndDrop;

    cleanupGarbage();
}

void WindowManager::cleanupGarbage()
{
    // Delete any dialogs which are no longer in use
    if (!garbageDialogs.empty())
    {
        for (std::vector<OEngine::GUI::Layout*>::iterator it = garbageDialogs.begin(); it != garbageDialogs.end(); ++it)
        {
            delete *it;
        }
        garbageDialogs.clear();
    }
}

void WindowManager::update()
{
    cleanupGarbage();
    if (needModeChange)
    {
        needModeChange = false;
        MWBase::Environment::get().getInputManager()->setGuiMode(nextMode);
        nextMode = GM_Game;
    }
    if (showFPSLevel > 0)
    {
        hud->setFPS(mFPS);
        hud->setTriangleCount(mTriangleCount);
        hud->setBatchCount(mBatchCount);
    }
}

void WindowManager::setNextMode(GuiMode newMode)
{
    nextMode = newMode;
    needModeChange = true;
}

void WindowManager::setGuiMode(GuiMode newMode)
{
    MWBase::Environment::get().getInputManager()->setGuiMode(newMode);
}

void WindowManager::updateVisible()
{
    // Start out by hiding everything except the HUD
    map->setVisible(false);
    menu->setVisible(false);
    stats->setVisible(false);
    console->disable();
    mJournal->setVisible(false);
    mDialogueWindow->setVisible(false);
    mContainerWindow->setVisible(false);
    mInventoryWindow->setVisible(false);

    // Mouse is visible whenever we're not in game mode
    MyGUI::PointerManager::getInstance().setVisible(isGuiMode());

    if (mode == GM_Game)
        mToolTips->enterGameMode();
    else
        mToolTips->enterGuiMode();

    switch(mode) {
        case GM_Game:
            // If in game mode, don't show anything.
            break;
        case GM_MainMenu:
            menu->setVisible(true);
            break;
        case GM_Console:
            console->enable();
            break;
        case GM_Name:
        case GM_Race:
        case GM_Class:
        case GM_ClassPick:
        case GM_ClassCreate:
        case GM_Birth:
        case GM_ClassGenerate:
        case GM_Review:
            mCharGen->spawnDialog(mode);
            break;
        case GM_Inventory:
        {
            // First, compute the effective set of windows to show.
            // This is controlled both by what windows the
            // user has opened/closed (the 'shown' variable) and by what
            // windows we are allowed to show (the 'allowed' var.)
            int eff = shown & allowed;

            // Show the windows we want
            map   -> setVisible( (eff & GW_Map) != 0 );
            stats -> setVisible( (eff & GW_Stats) != 0 );
            mInventoryWindow->setVisible(true);
            mInventoryWindow->openInventory();
            break;
        }
        case GM_Container:
            mContainerWindow->setVisible(true);
            mInventoryWindow->setVisible(true);
            mInventoryWindow->openInventory();
            break;
        case GM_Dialogue:
            mDialogueWindow->open();
            break;
        case GM_InterMessageBox:
            if(!mMessageBoxManager->isInteractiveMessageBox()) {
                setGuiMode(GM_Game);
            }
            break;
        case GM_Journal:
            mJournal->setVisible(true);
            mJournal->open();
            break;
        default:
            // Unsupported mode, switch back to game
            // Note: The call will eventually end up this method again but
            // will stop at the check if mode is GM_Game.
            setGuiMode(GM_Game);
            break;
    }
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    stats->setValue (id, value);

    static const char *ids[] =
    {
        "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
        "AttribVal6", "AttribVal7", "AttribVal8"
    };
    static ESM::Attribute::AttributeID attributes[] =
    {
        ESM::Attribute::Strength,
        ESM::Attribute::Intelligence,
        ESM::Attribute::Willpower,
        ESM::Attribute::Agility,
        ESM::Attribute::Speed,
        ESM::Attribute::Endurance,
        ESM::Attribute::Personality,
        ESM::Attribute::Luck
    };
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i)
    {
        if (id != ids[i])
            continue;
        playerAttributes[attributes[i]] = value;
        break;
    }
}


void WindowManager::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value)
{
    stats->setValue(parSkill, value);
    playerSkillValues[parSkill] = value;
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    stats->setValue (id, value);
    hud->setValue (id, value);
    if (id == "HBar")
    {
        playerHealth = value;
        mCharGen->setPlayerHealth (value);
    }
    else if (id == "MBar")
    {
        playerMagicka = value;
        mCharGen->setPlayerMagicka (value);
    }
    else if (id == "FBar")
    {
        playerFatigue = value;
        mCharGen->setPlayerFatigue (value);
    }
}

#if 0
MWMechanics::DynamicStat<int> WindowManager::getValue(const std::string& id)
{
    if(id == "HBar")
        return playerHealth;
    else if (id == "MBar")
        return playerMagicka;
    else if (id == "FBar")
        return playerFatigue;
}
#endif

void WindowManager::setValue (const std::string& id, const std::string& value)
{
    stats->setValue (id, value);
    if (id=="name")
        playerName = value;
    else if (id=="race")
        playerRaceId = value;
}

void WindowManager::setValue (const std::string& id, int value)
{
    stats->setValue (id, value);
}

void WindowManager::setPlayerClass (const ESM::Class &class_)
{
    playerClass = class_;
    stats->setValue("class", playerClass.name);
}

void WindowManager::configureSkills (const SkillList& major, const SkillList& minor)
{
    stats->configureSkills (major, minor);
    playerMajorSkills = major;
    playerMinorSkills = minor;
}

void WindowManager::setFactions (const FactionList& factions)
{
    stats->setFactions (factions);
}

void WindowManager::setBirthSign (const std::string &signId)
{
    stats->setBirthSign (signId);
    playerBirthSignId = signId;
}

void WindowManager::setReputation (int reputation)
{
    stats->setReputation (reputation);
}

void WindowManager::setBounty (int bounty)
{
    stats->setBounty (bounty);
}

void WindowManager::updateSkillArea()
{
    stats->updateSkillArea();
}

void WindowManager::removeDialog(OEngine::GUI::Layout*dialog)
{
    assert(dialog);
    if (!dialog)
        return;
    dialog->setVisible(false);
    garbageDialogs.push_back(dialog);
}

void WindowManager::messageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    if (buttons.empty())
    {
        mMessageBoxManager->createMessageBox(message);
    }
    else
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons);
        setGuiMode(GM_InterMessageBox);
    }
}

int WindowManager::readPressedButton ()
{
    return mMessageBoxManager->readPressedButton();
}

const std::string &WindowManager::getGameSettingString(const std::string &id, const std::string &default_)
{
    const ESM::GameSetting *setting = MWBase::Environment::get().getWorld()->getStore().gameSettings.search(id);
    if (setting && setting->type == ESM::VT_String)
        return setting->str;
    return default_;
}

void WindowManager::onDialogueWindowBye()
{
    if (mDialogueWindow)
    {
        //FIXME set some state and stuff?
        //removeDialog(dialogueWindow);
        mDialogueWindow->setVisible(false);
    }
    setGuiMode(GM_Game);
}

void WindowManager::onFrame (float frameDuration)
{
    mMessageBoxManager->onFrame(frameDuration);
    mInventoryWindow->Update();
    mContainerWindow->Update();
    mToolTips->onFrame(frameDuration);
}

const ESMS::ESMStore& WindowManager::getStore() const
{
    return MWBase::Environment::get().getWorld()->getStore();
}

void WindowManager::changeCell(MWWorld::Ptr::CellStore* cell)
{
    if (!(cell->cell->data.flags & ESM::Cell::Interior))
    {
        std::string name;
        if (cell->cell->name != "")
            name = cell->cell->name;
        else
        {
            const ESM::Region* region = MWBase::Environment::get().getWorld()->getStore().regions.search(cell->cell->region);
            name = region->name;
        }

        map->setCellName( name );

        map->setCellPrefix("Cell");
        hud->setCellPrefix("Cell");
        map->setActiveCell( cell->cell->data.gridX, cell->cell->data.gridY );
        hud->setActiveCell( cell->cell->data.gridX, cell->cell->data.gridY );
    }
    else
    {
        map->setCellName( cell->cell->name );
        map->setCellPrefix( cell->cell->name );
        hud->setCellPrefix( cell->cell->name );
    }

}

void WindowManager::setInteriorMapTexture(const int x, const int y)
{
    map->setActiveCell(x,y, true);
    hud->setActiveCell(x,y, true);
}

void WindowManager::setPlayerPos(const float x, const float y)
{
    map->setPlayerPos(x,y);
    hud->setPlayerPos(x,y);
}

void WindowManager::setPlayerDir(const float x, const float y)
{
    map->setPlayerDir(x,y);
    hud->setPlayerDir(x,y);
}

void WindowManager::setHMSVisibility(bool visible)
{
    hud->setBottomLeftVisibility(visible, hud->weapBox->getVisible(), hud->spellBox->getVisible());
}

void WindowManager::setMinimapVisibility(bool visible)
{
    hud->setBottomRightVisibility(hud->effectBox->getVisible(), visible);
}

void WindowManager::toggleFogOfWar()
{
    map->toggleFogOfWar();
    hud->toggleFogOfWar();
}

int WindowManager::toggleFps()
{
    showFPSLevel = (showFPSLevel+1)%3;
    hud->setFpsLevel(showFPSLevel);
    Settings::Manager::setInt("fps", "HUD", showFPSLevel);
    return showFPSLevel;
}

void WindowManager::setFocusObject(const MWWorld::Ptr& focus)
{
    mToolTips->setFocusObject(focus);
}

void WindowManager::toggleFullHelp()
{
    mToolTips->toggleFullHelp();
}

bool WindowManager::getFullHelp() const
{
    return mToolTips->getFullHelp();
}
