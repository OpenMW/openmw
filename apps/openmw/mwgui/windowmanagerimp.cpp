#include "windowmanagerimp.hpp"

#include <cassert>
#include <iterator>

#include "MyGUI_UString.h"

#include <openengine/ogre/renderer.hpp>
#include <openengine/gui/manager.hpp>

#include <components/settings/settings.hpp>
#include <components/translation/translation.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/cellstore.hpp"

#include "console.hpp"
#include "journalwindow.hpp"
#include "charactercreation.hpp"
#include "text_input.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "dialogue_history.hpp"
#include "map_window.hpp"
#include "stats_window.hpp"
#include "messagebox.hpp"
#include "container.hpp"
#include "inventorywindow.hpp"
#include "tooltips.hpp"
#include "scrollwindow.hpp"
#include "bookwindow.hpp"
#include "list.hpp"
#include "hud.hpp"
#include "mainmenu.hpp"
#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "spellbuyingwindow.hpp"
#include "travelwindow.hpp"
#include "settingswindow.hpp"
#include "confirmationdialog.hpp"
#include "alchemywindow.hpp"
#include "spellwindow.hpp"
#include "quickkeysmenu.hpp"
#include "loadingscreen.hpp"
#include "levelupdialog.hpp"
#include "waitdialog.hpp"
#include "spellcreationdialog.hpp"
#include "enchantingdialog.hpp"
#include "trainingwindow.hpp"
#include "imagebutton.hpp"

using namespace MWGui;

WindowManager::WindowManager(
    const Compiler::Extensions& extensions, int fpsLevel, bool newGame, OEngine::Render::OgreRenderer *mOgre,
        const std::string& logpath, const std::string& cacheDir, bool consoleOnlyScripts,
        Translation::Storage& translationDataStorage)
  : mGuiManager(NULL)
  , mHud(NULL)
  , mMap(NULL)
  , mMenu(NULL)
  , mStatsWindow(NULL)
  , mToolTips(NULL)
  , mMessageBoxManager(NULL)
  , mConsole(NULL)
  , mJournal(NULL)
  , mDialogueWindow(NULL)
  , mBookWindow(NULL)
  , mScrollWindow(NULL)
  , mCountDialog(NULL)
  , mTradeWindow(NULL)
  , mSpellBuyingWindow(NULL)
  , mTravelWindow(NULL)
  , mSettingsWindow(NULL)
  , mConfirmationDialog(NULL)
  , mAlchemyWindow(NULL)
  , mSpellWindow(NULL)
  , mLoadingScreen(NULL)
  , mCharGen(NULL)
  , mLevelupDialog(NULL)
  , mWaitDialog(NULL)
  , mSpellCreationDialog(NULL)
  , mEnchantingDialog(NULL)
  , mTrainingWindow(NULL)
  , mPlayerName()
  , mPlayerRaceId()
  , mPlayerAttributes()
  , mPlayerMajorSkills()
  , mPlayerMinorSkills()
  , mPlayerSkillValues()
  , mPlayerHealth()
  , mPlayerMagicka()
  , mPlayerFatigue()
  , mGui(NULL)
  , mGarbageDialogs()
  , mShown(GW_ALL)
  , mAllowed(newGame ? GW_None : GW_ALL)
  , mRestAllowed(newGame ? false : true)
  , mShowFPSLevel(fpsLevel)
  , mFPS(0.0f)
  , mTriangleCount(0)
  , mBatchCount(0)
  , mCrosshairEnabled(Settings::Manager::getBool ("crosshair", "HUD"))
  , mSubtitlesEnabled(Settings::Manager::getBool ("subtitles", "GUI"))
  , mHudEnabled(true)
  , mTranslationDataStorage (translationDataStorage)
{

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre->getWindow(), mOgre->getScene(), false, logpath);
    mGui = mGuiManager->getGui();

    //Register own widgets with MyGUI
    MyGUI::FactoryManager::getInstance().registerFactory<DialogueHistory>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWEffectList>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWList>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::HBox>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::VBox>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::AutoSizedTextBox>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::AutoSizedButton>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ImageButton>("Widget");

    MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &WindowManager::onRetrieveTag);

    // Get size info from the Gui object
    assert(mGui);
    int w = MyGUI::RenderManager::getInstance().getViewSize().width;
    int h = MyGUI::RenderManager::getInstance().getViewSize().height;

    MyGUI::Widget* dragAndDropWidget = mGui->createWidgetT("Widget","",0,0,w,h,MyGUI::Align::Default,"DragAndDrop","DragAndDropWidget");
    dragAndDropWidget->setVisible(false);

    mDragAndDrop = new DragAndDrop();
    mDragAndDrop->mIsOnDragAndDrop = false;
    mDragAndDrop->mDraggedWidget = 0;
    mDragAndDrop->mDragAndDropWidget = dragAndDropWidget;

    mMenu = new MainMenu(w,h);
    mMap = new MapWindow(*this, cacheDir);
    mStatsWindow = new StatsWindow(*this);
    mConsole = new Console(w,h, consoleOnlyScripts);
    mJournal = new JournalWindow(*this);
    mMessageBoxManager = new MessageBoxManager(this);
    mInventoryWindow = new InventoryWindow(*this,mDragAndDrop);
    mTradeWindow = new TradeWindow(*this);
    mSpellBuyingWindow = new SpellBuyingWindow(*this);
    mTravelWindow = new TravelWindow(*this);
    mDialogueWindow = new DialogueWindow(*this);
    mContainerWindow = new ContainerWindow(*this,mDragAndDrop);
    mHud = new HUD(w,h, mShowFPSLevel, mDragAndDrop);
    mToolTips = new ToolTips(this);
    mScrollWindow = new ScrollWindow(*this);
    mBookWindow = new BookWindow(*this);
    mCountDialog = new CountDialog(*this);
    mSettingsWindow = new SettingsWindow(*this);
    mConfirmationDialog = new ConfirmationDialog(*this);
    mAlchemyWindow = new AlchemyWindow(*this);
    mSpellWindow = new SpellWindow(*this);
    mQuickKeysMenu = new QuickKeysMenu(*this);
    mLevelupDialog = new LevelupDialog(*this);
    mWaitDialog = new WaitDialog(*this);
    mSpellCreationDialog = new SpellCreationDialog(*this);
    mEnchantingDialog = new EnchantingDialog(*this);
    mTrainingWindow = new TrainingWindow(*this);

    mLoadingScreen = new LoadingScreen(mOgre->getScene (), mOgre->getWindow (), *this);
    mLoadingScreen->onResChange (w,h);

    mInputBlocker = mGui->createWidget<MyGUI::Widget>("",0,0,w,h,MyGUI::Align::Default,"Windows","");

    // The HUD is always on
    mHud->setVisible(true);

    mCharGen = new CharacterCreation(this);

    // Setup player stats
    for (int i = 0; i < ESM::Attribute::Length; ++i)
    {
        mPlayerAttributes.insert(std::make_pair(ESM::Attribute::sAttributeIds[i], MWMechanics::Stat<int>()));
    }

    for (int i = 0; i < ESM::Skill::Length; ++i)
    {
        mPlayerSkillValues.insert(std::make_pair(ESM::Skill::sSkillIds[i], MWMechanics::Stat<float>()));
    }

    unsetSelectedSpell();
    unsetSelectedWeapon();

    // Set up visibility
    updateVisible();
}

WindowManager::~WindowManager()
{
    delete mConsole;
    delete mMessageBoxManager;
    delete mHud;
    delete mMap;
    delete mMenu;
    delete mStatsWindow;
    delete mJournal;
    delete mDialogueWindow;
    delete mContainerWindow;
    delete mInventoryWindow;
    delete mToolTips;
    delete mCharGen;
    delete mDragAndDrop;
    delete mBookWindow;
    delete mScrollWindow;
    delete mTradeWindow;
    delete mSpellBuyingWindow;
    delete mTravelWindow;
    delete mSettingsWindow;
    delete mConfirmationDialog;
    delete mAlchemyWindow;
    delete mSpellWindow;
    delete mLoadingScreen;
    delete mLevelupDialog;
    delete mWaitDialog;
    delete mSpellCreationDialog;
    delete mEnchantingDialog;
    delete mTrainingWindow;
    delete mCountDialog;
    delete mQuickKeysMenu;

    cleanupGarbage();

    delete mGuiManager;
}

void WindowManager::cleanupGarbage()
{
    // Delete any dialogs which are no longer in use
    if (!mGarbageDialogs.empty())
    {
        for (std::vector<OEngine::GUI::Layout*>::iterator it = mGarbageDialogs.begin(); it != mGarbageDialogs.end(); ++it)
        {
            delete *it;
        }
        mGarbageDialogs.clear();
    }
}

void WindowManager::update()
{
    cleanupGarbage();

    mHud->setFPS(mFPS);
    mHud->setTriangleCount(mTriangleCount);
    mHud->setBatchCount(mBatchCount);
}

void WindowManager::updateVisible()
{
    // Start out by hiding everything except the HUD
    mMap->setVisible(false);
    mMenu->setVisible(false);
    mStatsWindow->setVisible(false);
    mConsole->disable();
    mJournal->setVisible(false);
    mDialogueWindow->setVisible(false);
    mContainerWindow->setVisible(false);
    mInventoryWindow->setVisible(false);
    mScrollWindow->setVisible(false);
    mBookWindow->setVisible(false);
    mTradeWindow->setVisible(false);
    mSpellBuyingWindow->setVisible(false);
    mTravelWindow->setVisible(false);
    mSettingsWindow->setVisible(false);
    mAlchemyWindow->setVisible(false);
    mSpellWindow->setVisible(false);
    mQuickKeysMenu->setVisible(false);
    mLevelupDialog->setVisible(false);
    mWaitDialog->setVisible(false);
    mSpellCreationDialog->setVisible(false);
    mEnchantingDialog->setVisible(false);
    mTrainingWindow->setVisible(false);

    mHud->setVisible(true);

    // Mouse is visible whenever we're not in game mode
    MyGUI::PointerManager::getInstance().setVisible(isGuiMode());

    bool gameMode = !isGuiMode();

    mInputBlocker->setVisible (gameMode);

    if (gameMode)
        mToolTips->enterGameMode();
    else
        mToolTips->enterGuiMode();

    if (gameMode)
        MyGUI::InputManager::getInstance ().setKeyFocusWidget (NULL);

    setMinimapVisibility((mAllowed & GW_Map) && !mMap->pinned());
    setWeaponVisibility((mAllowed & GW_Inventory) && !mInventoryWindow->pinned());
    setSpellVisibility((mAllowed & GW_Magic) && !mSpellWindow->pinned());
    setHMSVisibility((mAllowed & GW_Stats) && !mStatsWindow->pinned());

    // If in game mode, don't show anything.
    if (gameMode)
        return;

    GuiMode mode = mGuiModes.back();

    switch(mode) {
        case GM_QuickKeysMenu:
            mQuickKeysMenu->setVisible (true);
            break;
        case GM_MainMenu:
            mMenu->setVisible(true);
            break;
        case GM_Settings:
            mSettingsWindow->setVisible(true);
            break;
        case GM_Console:
            mConsole->enable();
            break;
        case GM_Scroll:
            mScrollWindow->setVisible(true);
            break;
        case GM_Book:
            mBookWindow->setVisible(true);
            break;
        case GM_Alchemy:
            mAlchemyWindow->setVisible(true);
            break;
        case GM_Rest:
            mWaitDialog->setVisible(true);
            break;
        case GM_RestBed:
            mWaitDialog->setVisible(true);
            mWaitDialog->bedActivated();
            break;
        case GM_Levelup:
            mLevelupDialog->setVisible(true);
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
            int eff = mShown & mAllowed;

            // Show the windows we want
            mMap            ->setVisible(eff & GW_Map);
            mStatsWindow    ->setVisible(eff & GW_Stats);
            mInventoryWindow->setVisible(eff & GW_Inventory);
            mSpellWindow    ->setVisible(eff & GW_Magic);
            break;
        }
        case GM_Container:
            mContainerWindow->setVisible(true);
            mInventoryWindow->setVisible(true);
            break;
        case GM_Dialogue:
            mDialogueWindow->setVisible(true);
            break;
        case GM_Barter:
            mInventoryWindow->setVisible(true);
            mTradeWindow->setVisible(true);
            break;
        case GM_SpellBuying:
            mSpellBuyingWindow->setVisible(true);
            break;
        case GM_Travel:
            mTravelWindow->setVisible(true);
            break;
        case GM_SpellCreation:
            mSpellCreationDialog->setVisible(true);
            break;
        case GM_Enchanting:
            mEnchantingDialog->setVisible(true);
            break;
        case GM_Training:
            mTrainingWindow->setVisible(true);
            break;
        case GM_InterMessageBox:
            break;
        case GM_Journal:
            mJournal->setVisible(true);
            break;
        case GM_LoadingWallpaper:
            mHud->setVisible(false);
            MyGUI::PointerManager::getInstance().setVisible(false);
            break;
        case GM_Loading:
            MyGUI::PointerManager::getInstance().setVisible(false);
            break;
        case GM_Video:
            MyGUI::PointerManager::getInstance().setVisible(false);
            mHud->setVisible(false);
            break;
        default:
            // Unsupported mode, switch back to game
            break;
    }
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    mStatsWindow->setValue (id, value);
    mCharGen->setValue(id, value);

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
        mPlayerAttributes[attributes[i]] = value;
        break;
    }
}


void WindowManager::setValue (int parSkill, const MWMechanics::Stat<float>& value)
{
    /// \todo Don't use the skill enum as a parameter type (we will have to drop it anyway, once we
    /// allow custom skills.
    mStatsWindow->setValue(static_cast<ESM::Skill::SkillEnum> (parSkill), value);
    mCharGen->setValue(static_cast<ESM::Skill::SkillEnum> (parSkill), value);
    mPlayerSkillValues[parSkill] = value;
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value)
{
    mStatsWindow->setValue (id, value);
    mHud->setValue (id, value);
    mCharGen->setValue(id, value);
    if (id == "HBar")
    {
        mPlayerHealth = value;
        mCharGen->setPlayerHealth (value);
    }
    else if (id == "MBar")
    {
        mPlayerMagicka = value;
        mCharGen->setPlayerMagicka (value);
    }
    else if (id == "FBar")
    {
        mPlayerFatigue = value;
        mCharGen->setPlayerFatigue (value);
    }
}

#if 0
MWMechanics::DynamicStat<int> WindowManager::getValue(const std::string& id)
{
    if(id == "HBar")
        return layerHealth;
    else if (id == "MBar")
        return mPlayerMagicka;
    else if (id == "FBar")
        return mPlayerFatigue;
}
#endif

void WindowManager::setValue (const std::string& id, const std::string& value)
{
    mStatsWindow->setValue (id, value);
    if (id=="name")
        mPlayerName = value;
    else if (id=="race")
        mPlayerRaceId = value;
}

void WindowManager::setValue (const std::string& id, int value)
{
    mStatsWindow->setValue (id, value);
}

void WindowManager::setPlayerClass (const ESM::Class &class_)
{
    mStatsWindow->setValue("class", class_.mName);
}

void WindowManager::configureSkills (const SkillList& major, const SkillList& minor)
{
    mStatsWindow->configureSkills (major, minor);
    mCharGen->configureSkills(major, minor);
    mPlayerMajorSkills = major;
    mPlayerMinorSkills = minor;
}

void WindowManager::setReputation (int reputation)
{
    mStatsWindow->setReputation (reputation);
}

void WindowManager::setBounty (int bounty)
{
    mStatsWindow->setBounty (bounty);
}

void WindowManager::updateSkillArea()
{
    mStatsWindow->updateSkillArea();
}

void WindowManager::removeDialog(OEngine::GUI::Layout*dialog)
{
    if (!dialog)
        return;
    dialog->setVisible(false);
    mGarbageDialogs.push_back(dialog);
}

void WindowManager::messageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    if(buttons.empty()){
        /* If there are no buttons, and there is a dialogue window open, messagebox goes to the dialogue window */
        if(!mGuiModes.empty() && mGuiModes.back() == GM_Dialogue)
            mDialogueWindow->addMessageBox(MyGUI::LanguageManager::getInstance().replaceTags(message));
        else
            mMessageBoxManager->createMessageBox(message);
    }
    
    else
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons);
        pushGuiMode(GM_InterMessageBox);
    }
}

int WindowManager::readPressedButton ()
{
    return mMessageBoxManager->readPressedButton();
}

std::string WindowManager::getGameSettingString(const std::string &id, const std::string &default_)
{
    const ESM::GameSetting *setting =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().search(id);

    if (setting && setting->mType == ESM::VT_String)
        return setting->getString();
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
    removeGuiMode(GM_Dialogue);
}

void WindowManager::onFrame (float frameDuration)
{
    mMessageBoxManager->onFrame(frameDuration);
    mToolTips->onFrame(frameDuration);

    if (mDragAndDrop->mIsOnDragAndDrop)
    {
        assert(mDragAndDrop->mDraggedWidget);
        mDragAndDrop->mDraggedWidget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
    }

    mDialogueWindow->onFrame();

    mInventoryWindow->onFrame();

    mStatsWindow->onFrame();

    mWaitDialog->onFrame(frameDuration);

    mHud->onFrame(frameDuration);

    mTrainingWindow->onFrame (frameDuration);

    mTrainingWindow->checkReferenceAvailable();
    mDialogueWindow->checkReferenceAvailable();
    mTradeWindow->checkReferenceAvailable();
    mSpellBuyingWindow->checkReferenceAvailable();
    mSpellCreationDialog->checkReferenceAvailable();
    mEnchantingDialog->checkReferenceAvailable();
    mContainerWindow->checkReferenceAvailable();
    mConsole->checkReferenceAvailable();
}

void WindowManager::changeCell(MWWorld::Ptr::CellStore* cell)
{
    if (cell->mCell->isExterior())
    {
        std::string name;
        if (cell->mCell->mName != "")
        {
            name = cell->mCell->mName;
            mMap->addVisitedLocation ("#{sCell=" + name + "}", cell->mCell->getGridX (), cell->mCell->getGridY ());
        }
        else
        {
            const ESM::Region* region =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Region>().search(cell->mCell->mRegion);
            if (region)
                name = region->mName;
            else
                name = getGameSettingString("sDefaultCellname", "Wilderness");
        }

        mMap->cellExplored(cell->mCell->getGridX(), cell->mCell->getGridY());

        mMap->setCellName( name );
        mHud->setCellName( name );

        mMap->setCellPrefix("Cell");
        mHud->setCellPrefix("Cell");
        mMap->setActiveCell( cell->mCell->getGridX(), cell->mCell->getGridY() );
        mHud->setActiveCell( cell->mCell->getGridX(), cell->mCell->getGridY() );
    }
    else
    {
        mMap->setCellName( cell->mCell->mName );
        mHud->setCellName( cell->mCell->mName );
        mMap->setCellPrefix( cell->mCell->mName );
        mHud->setCellPrefix( cell->mCell->mName );
    }

}

void WindowManager::setInteriorMapTexture(const int x, const int y)
{
    mMap->setActiveCell(x,y, true);
    mHud->setActiveCell(x,y, true);
}

void WindowManager::setPlayerPos(const float x, const float y)
{
    mMap->setPlayerPos(x,y);
    mHud->setPlayerPos(x,y);
}

void WindowManager::setPlayerDir(const float x, const float y)
{
    mMap->setPlayerDir(x,y);
    mHud->setPlayerDir(x,y);
}

void WindowManager::setHMSVisibility(bool visible)
{
    mHud->setHmsVisible (visible);
}

void WindowManager::setMinimapVisibility(bool visible)
{
    mHud->setMinimapVisible (visible);
}

void WindowManager::toggleFogOfWar()
{
    mMap->toggleFogOfWar();
    mHud->toggleFogOfWar();
}

void WindowManager::setFocusObject(const MWWorld::Ptr& focus)
{
    mToolTips->setFocusObject(focus);
}

void WindowManager::setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y)
{
    mToolTips->setFocusObjectScreenCoords(min_x, min_y, max_x, max_y);
}

void WindowManager::toggleFullHelp()
{
    mToolTips->toggleFullHelp();
}

bool WindowManager::getFullHelp() const
{
    return mToolTips->getFullHelp();
}

void WindowManager::setWeaponVisibility(bool visible)
{
    mHud->setWeapVisible (visible);
}

void WindowManager::setSpellVisibility(bool visible)
{
    mHud->setSpellVisible (visible);
    mHud->setEffectVisible (visible);
}

void WindowManager::setMouseVisible(bool visible)
{
    MyGUI::PointerManager::getInstance().setVisible(visible);
}

void WindowManager::setDragDrop(bool dragDrop)
{
    mToolTips->setEnabled(!dragDrop);
    MWBase::Environment::get().getInputManager()->setDragDrop(dragDrop);
}

void WindowManager::onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
{
    std::string tag(_tag);

    std::string tokenToFind = "sCell=";
    size_t tokenLength = tokenToFind.length();

    if (tag.substr(0, tokenLength) == tokenToFind)
    {
        _result = mTranslationDataStorage.translateCellName(tag.substr(tokenLength));
    }
    else
    {
        const ESM::GameSetting *setting =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(tag);

        if (setting && setting->mType == ESM::VT_String)
            _result = setting->getString();
        else
            _result = tag;
    }
}

void WindowManager::processChangedSettings(const Settings::CategorySettingVector& changed)
{
    mHud->setFpsLevel(Settings::Manager::getInt("fps", "HUD"));
    mToolTips->setDelay(Settings::Manager::getFloat("tooltip delay", "GUI"));

    bool changeRes = false;
    for (Settings::CategorySettingVector::const_iterator it = changed.begin();
        it != changed.end(); ++it)
    {
        if (it->first == "Video" &&  (
            it->second == "resolution x"
            || it->second == "resolution y"))
        {
            changeRes = true;
        }
        else if (it->first == "HUD" && it->second == "crosshair")
            mCrosshairEnabled = Settings::Manager::getBool ("crosshair", "HUD");
        else if (it->first == "GUI" && it->second == "subtitles")
            mSubtitlesEnabled = Settings::Manager::getBool ("subtitles", "GUI");
    }

    if (changeRes)
    {
        int x = Settings::Manager::getInt("resolution x", "Video");
        int y = Settings::Manager::getInt("resolution y", "Video");
        mHud->onResChange(x, y);
        mConsole->onResChange(x, y);
        mMenu->onResChange(x, y);
        mSettingsWindow->center();
        mAlchemyWindow->center();
        mScrollWindow->center();
        mBookWindow->center();
        mQuickKeysMenu->center();
        mSpellBuyingWindow->center();
        mLoadingScreen->onResChange (x,y);
        mDragAndDrop->mDragAndDropWidget->setSize(MyGUI::IntSize(x, y));
        mInputBlocker->setSize(MyGUI::IntSize(x,y));
    }
}

void WindowManager::pushGuiMode(GuiMode mode)
{
    if (mode==GM_Inventory && mAllowed==GW_None)
        return;


    // If this mode already exists somewhere in the stack, just bring it to the front.
    if (std::find(mGuiModes.begin(), mGuiModes.end(), mode) != mGuiModes.end())
    {
        mGuiModes.erase(std::find(mGuiModes.begin(), mGuiModes.end(), mode));
    }

    mGuiModes.push_back(mode);

    bool gameMode = !isGuiMode();
    MWBase::Environment::get().getInputManager()->changeInputMode(!gameMode);

    updateVisible();
}

void WindowManager::popGuiMode()
{
    if (!mGuiModes.empty())
        mGuiModes.pop_back();

    bool gameMode = !isGuiMode();
    MWBase::Environment::get().getInputManager()->changeInputMode(!gameMode);

    updateVisible();
}

void WindowManager::removeGuiMode(GuiMode mode)
{
    std::vector<GuiMode>::iterator it = mGuiModes.begin();
    while (it != mGuiModes.end())
    {
        if (*it == mode)
            it = mGuiModes.erase(it);
        else
            ++it;
    }

    bool gameMode = !isGuiMode();
    MWBase::Environment::get().getInputManager()->changeInputMode(!gameMode);

    updateVisible();
}

void WindowManager::setSelectedSpell(const std::string& spellId, int successChancePercent)
{
    mHud->setSelectedSpell(spellId, successChancePercent);

    const ESM::Spell* spell =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

    mSpellWindow->setTitle(spell->mName);
}

void WindowManager::setSelectedEnchantItem(const MWWorld::Ptr& item, int chargePercent)
{
    mHud->setSelectedEnchantItem(item, chargePercent);
    mSpellWindow->setTitle(MWWorld::Class::get(item).getName(item));
}

void WindowManager::setSelectedWeapon(const MWWorld::Ptr& item, int durabilityPercent)
{
    mHud->setSelectedWeapon(item, durabilityPercent);
    mInventoryWindow->setTitle(MWWorld::Class::get(item).getName(item));
}

void WindowManager::unsetSelectedSpell()
{
    mHud->unsetSelectedSpell();
    mSpellWindow->setTitle("#{sNone}");
}

void WindowManager::unsetSelectedWeapon()
{
    mHud->unsetSelectedWeapon();
    mInventoryWindow->setTitle("#{sSkillHandtohand}");
}

void WindowManager::getMousePosition(int &x, int &y)
{
    const MyGUI::IntPoint& pos = MyGUI::InputManager::getInstance().getMousePosition();
    x = pos.left;
    y = pos.top;
}

void WindowManager::getMousePosition(float &x, float &y)
{
    const MyGUI::IntPoint& pos = MyGUI::InputManager::getInstance().getMousePosition();
    x = pos.left;
    y = pos.top;
    const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
    x /= viewSize.width;
    y /= viewSize.height;
}

bool WindowManager::getWorldMouseOver()
{
    return mHud->getWorldMouseOver();
}

void WindowManager::executeInConsole (const std::string& path)
{
    mConsole->executeFile (path);
}

void WindowManager::wmUpdateFps(float fps, unsigned int triangleCount, unsigned int batchCount)
{
    mFPS = fps;
    mTriangleCount = triangleCount;
    mBatchCount = batchCount;
}

MyGUI::Gui* WindowManager::getGui() const { return mGui; }

MWGui::DialogueWindow* WindowManager::getDialogueWindow() { return mDialogueWindow;  }
MWGui::ContainerWindow* WindowManager::getContainerWindow() { return mContainerWindow; }
MWGui::InventoryWindow* WindowManager::getInventoryWindow() { return mInventoryWindow; }
MWGui::BookWindow* WindowManager::getBookWindow() { return mBookWindow; }
MWGui::ScrollWindow* WindowManager::getScrollWindow() { return mScrollWindow; }
MWGui::CountDialog* WindowManager::getCountDialog() { return mCountDialog; }
MWGui::ConfirmationDialog* WindowManager::getConfirmationDialog() { return mConfirmationDialog; }
MWGui::TradeWindow* WindowManager::getTradeWindow() { return mTradeWindow; }
MWGui::SpellBuyingWindow* WindowManager::getSpellBuyingWindow() { return mSpellBuyingWindow; }
MWGui::TravelWindow* WindowManager::getTravelWindow() { return mTravelWindow; }
MWGui::SpellWindow* WindowManager::getSpellWindow() { return mSpellWindow; }
MWGui::Console* WindowManager::getConsole() { return mConsole; }

bool WindowManager::isAllowed (GuiWindow wnd) const
{
    return mAllowed & wnd;
}

void WindowManager::allow (GuiWindow wnd)
{
    mAllowed = (GuiWindow)(mAllowed | wnd);
    updateVisible();
}

void WindowManager::disallowAll()
{
    mAllowed = GW_None;
    updateVisible();
}

void WindowManager::toggleVisible (GuiWindow wnd)
{
    mShown = (mShown & wnd) ? (GuiWindow) (mShown & ~wnd) : (GuiWindow) (mShown | wnd);
    updateVisible();
}

bool WindowManager::isGuiMode() const
{
    return !mGuiModes.empty();
}

MWGui::GuiMode WindowManager::getMode() const
{
    if (mGuiModes.empty())
        throw std::runtime_error ("getMode() called, but there is no active mode");

    return mGuiModes.back();
}

std::map<int, MWMechanics::Stat<float> > WindowManager::getPlayerSkillValues()
{
    return mPlayerSkillValues;
}

std::map<int, MWMechanics::Stat<int> > WindowManager::getPlayerAttributeValues()
{
    return mPlayerAttributes;
}

WindowManager::SkillList WindowManager::getPlayerMinorSkills()
{
    return mPlayerMinorSkills;
}

WindowManager::SkillList WindowManager::getPlayerMajorSkills()
{
    return mPlayerMajorSkills;
}

void WindowManager::disallowMouse()
{
    mInputBlocker->setVisible (true);
}

void WindowManager::allowMouse()
{
    mInputBlocker->setVisible (!isGuiMode ());
}

void WindowManager::notifyInputActionBound ()
{
    mSettingsWindow->updateControlsBox ();
    allowMouse();
}


void WindowManager::showCrosshair (bool show)
{
    mHud->setCrosshairVisible (show && mCrosshairEnabled);
}

void WindowManager::activateQuickKey (int index)
{
    mQuickKeysMenu->activateQuickKey(index);
}

bool WindowManager::getSubtitlesEnabled ()
{
    return mSubtitlesEnabled;
}

void WindowManager::toggleHud ()
{
    mHudEnabled = !mHudEnabled;
    mHud->setVisible (mHudEnabled);
}

void WindowManager::setLoadingProgress (const std::string& stage, int depth, int current, int total)
{
    mLoadingScreen->setLoadingProgress (stage, depth, current, total);
}

void WindowManager::loadingDone ()
{
    mLoadingScreen->loadingDone ();
}

bool WindowManager::getPlayerSleeping ()
{
    return mWaitDialog->getSleeping();
}

void WindowManager::wakeUpPlayer()
{
    mWaitDialog->wakeUp();
}

void WindowManager::addVisitedLocation(const std::string& name, int x, int y)
{
    mMap->addVisitedLocation (name, x, y);
}

void WindowManager::startSpellMaking(MWWorld::Ptr actor)
{
    mSpellCreationDialog->startSpellMaking (actor);
}

void WindowManager::startEnchanting (MWWorld::Ptr actor)
{
    mEnchantingDialog->startEnchanting (actor);
}

void WindowManager::startTraining(MWWorld::Ptr actor)
{
    mTrainingWindow->startTraining(actor);
}

const Translation::Storage& WindowManager::getTranslationDataStorage() const
{
    return mTranslationDataStorage;
}
