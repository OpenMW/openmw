#include "windowmanagerimp.hpp"

#include <cassert>
#include <iterator>

#include <OgreTextureManager.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>

#include <MyGUI_UString.h>
#include <MyGUI_IPointer.h>
#include <MyGUI_ResourceImageSetPointer.h>
#include <MyGUI_TextureUtility.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_PointerManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ClipboardManager.h>
#include <MyGUI_RenderManager.h>

#include <SDL_keyboard.h>
#include <SDL_clipboard.h>

#include <openengine/ogre/renderer.hpp>
#include <openengine/gui/manager.hpp>

#include <extern/sdl4ogre/sdlcursormanager.hpp>

#include <components/fontloader/fontloader.hpp>

#include <components/translation/translation.hpp>

#include <components/widgets/widgets.hpp>
#include <components/widgets/tags.hpp>

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/stat.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwsound/soundmanagerimp.hpp"

#include "console.hpp"
#include "journalwindow.hpp"
#include "journalviewmodel.hpp"
#include "charactercreation.hpp"
#include "dialogue.hpp"
#include "statswindow.hpp"
#include "messagebox.hpp"
#include "tooltips.hpp"
#include "scrollwindow.hpp"
#include "bookwindow.hpp"
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
#include "enchantingdialog.hpp"
#include "trainingwindow.hpp"
#include "recharge.hpp"
#include "exposedwindow.hpp"
#include "cursor.hpp"
#include "merchantrepair.hpp"
#include "repair.hpp"
#include "soulgemdialog.hpp"
#include "companionwindow.hpp"
#include "inventorywindow.hpp"
#include "bookpage.hpp"
#include "itemview.hpp"
#include "videowidget.hpp"
#include "backgroundimage.hpp"
#include "itemwidget.hpp"
#include "screenfader.hpp"
#include "debugwindow.hpp"
#include "spellview.hpp"
#include "draganddrop.hpp"
#include "container.hpp"
#include "controllers.hpp"
#include "jailscreen.hpp"

namespace MWGui
{

    WindowManager::WindowManager(
        const Compiler::Extensions& extensions, OEngine::Render::OgreRenderer *ogre,
            const std::string& logpath, const std::string& cacheDir, bool consoleOnlyScripts,
            Translation::Storage& translationDataStorage, ToUTF8::FromType encoding, bool exportFonts, const std::map<std::string, std::string>& fallbackMap)
      : mConsoleOnlyScripts(consoleOnlyScripts)
      , mCurrentModals()
      , mGuiManager(NULL)
      , mRendering(ogre)
      , mHud(NULL)
      , mMap(NULL)
      , mMenu(NULL)
      , mToolTips(NULL)
      , mStatsWindow(NULL)
      , mMessageBoxManager(NULL)
      , mConsole(NULL)
      , mJournal(NULL)
      , mDialogueWindow(NULL)
      , mContainerWindow(NULL)
      , mDragAndDrop(NULL)
      , mInventoryWindow(NULL)
      , mScrollWindow(NULL)
      , mBookWindow(NULL)
      , mCountDialog(NULL)
      , mTradeWindow(NULL)
      , mSpellBuyingWindow(NULL)
      , mTravelWindow(NULL)
      , mSettingsWindow(NULL)
      , mConfirmationDialog(NULL)
      , mAlchemyWindow(NULL)
      , mSpellWindow(NULL)
      , mQuickKeysMenu(NULL)
      , mLoadingScreen(NULL)
      , mLevelupDialog(NULL)
      , mWaitDialog(NULL)
      , mSpellCreationDialog(NULL)
      , mEnchantingDialog(NULL)
      , mTrainingWindow(NULL)
      , mMerchantRepair(NULL)
      , mSoulgemDialog(NULL)
      , mRepair(NULL)
      , mRecharge(NULL)
      , mCompanionWindow(NULL)
      , mVideoBackground(NULL)
      , mVideoWidget(NULL)
      , mWerewolfFader(NULL)
      , mBlindnessFader(NULL)
      , mHitFader(NULL)
      , mScreenFader(NULL)
      , mDebugWindow(NULL)
      , mJailScreen(NULL)
      , mTranslationDataStorage (translationDataStorage)
      , mCharGen(NULL)
      , mInputBlocker(NULL)
      , mCrosshairEnabled(Settings::Manager::getBool ("crosshair", "HUD"))
      , mSubtitlesEnabled(Settings::Manager::getBool ("subtitles", "GUI"))
      , mHitFaderEnabled(Settings::Manager::getBool ("hit fader", "GUI"))
      , mWerewolfOverlayEnabled(Settings::Manager::getBool ("werewolf overlay", "GUI"))
      , mHudEnabled(true)
      , mGuiEnabled(true)
      , mCursorVisible(true)
      , mPlayerName()
      , mPlayerRaceId()
      , mPlayerAttributes()
      , mPlayerMajorSkills()
      , mPlayerMinorSkills()
      , mPlayerSkillValues()
      , mGui(NULL)
      , mGuiModes()
      , mCursorManager(NULL)
      , mGarbageDialogs()
      , mShown(GW_ALL)
      , mForceHidden(GW_None)
      , mAllowed(GW_ALL)
      , mRestAllowed(true)
      , mFPS(0.0f)
      , mTriangleCount(0)
      , mBatchCount(0)
      , mFallbackMap(fallbackMap)
    {
        // Set up the GUI system
        mGuiManager = new OEngine::GUI::MyGUIManager(mRendering->getWindow(), mRendering->getScene(), false, logpath);

        MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &WindowManager::onRetrieveTag);

        // Load fonts
        Gui::FontLoader fontLoader (encoding);
        fontLoader.loadAllFonts(exportFonts);

        //Register own widgets with MyGUI
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWEffectList>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ExposedWindow>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWScrollBar>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<VideoWidget>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<BackgroundImage>("Widget");
        BookPage::registerMyGUIComponents ();
        ItemView::registerComponents();
        ItemWidget::registerComponents();
        SpellView::registerComponents();
        Gui::registerAllWidgets();

        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Controllers::ControllerRepeatEvent>("Controller");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Controllers::ControllerFollowMouse>("Controller");

        MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
        MyGUI::ResourceManager::getInstance().load("core.xml");

        mLoadingScreen = new LoadingScreen(mRendering->getScene (), mRendering->getWindow ());

        //set up the hardware cursor manager
        mCursorManager = new SFO::SDLCursorManager();

        MyGUI::PointerManager::getInstance().eventChangeMousePointer += MyGUI::newDelegate(this, &WindowManager::onCursorChange);

        MyGUI::InputManager::getInstance().eventChangeKeyFocus += MyGUI::newDelegate(this, &WindowManager::onKeyFocusChanged);

        onCursorChange(MyGUI::PointerManager::getInstance().getDefaultPointer());

        mCursorManager->setEnabled(true);

        // hide mygui's pointer
        MyGUI::PointerManager::getInstance().setVisible(false);

        mVideoBackground = MyGUI::Gui::getInstance().createWidgetReal<MyGUI::ImageBox>("ImageBox", 0,0,1,1,
            MyGUI::Align::Default, "Overlay");
        mVideoBackground->setImageTexture("black.png");
        mVideoBackground->setVisible(false);
        mVideoBackground->setNeedMouseFocus(true);
        mVideoBackground->setNeedKeyFocus(true);

        mVideoWidget = mVideoBackground->createWidgetReal<VideoWidget>("ImageBox", 0,0,1,1, MyGUI::Align::Default);
        mVideoWidget->setNeedMouseFocus(true);
        mVideoWidget->setNeedKeyFocus(true);

        // Removes default MyGUI system clipboard implementation, which supports windows only
        MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

        MyGUI::ClipboardManager::getInstance().eventClipboardChanged += MyGUI::newDelegate(this, &WindowManager::onClipboardChanged);
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested += MyGUI::newDelegate(this, &WindowManager::onClipboardRequested);
    }

    void WindowManager::initUI()
    {
        // Get size info from the Gui object
        int w = MyGUI::RenderManager::getInstance().getViewSize().width;
        int h = MyGUI::RenderManager::getInstance().getViewSize().height;

        mDragAndDrop = new DragAndDrop();

        mRecharge = new Recharge();
        mMenu = new MainMenu(w,h);
        mMap = new MapWindow(mCustomMarkers, mDragAndDrop, "");
        trackWindow(mMap, "map");
        mStatsWindow = new StatsWindow(mDragAndDrop);
        trackWindow(mStatsWindow, "stats");
        mConsole = new Console(w,h, mConsoleOnlyScripts);
        trackWindow(mConsole, "console");
        mJournal = JournalWindow::create(JournalViewModel::create ());
        mMessageBoxManager = new MessageBoxManager(
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fMessageTimePerChar")->getFloat());
        mInventoryWindow = new InventoryWindow(mDragAndDrop);
        mTradeWindow = new TradeWindow();
        trackWindow(mTradeWindow, "barter");
        mSpellBuyingWindow = new SpellBuyingWindow();
        mTravelWindow = new TravelWindow();
        mDialogueWindow = new DialogueWindow();
        trackWindow(mDialogueWindow, "dialogue");
        mContainerWindow = new ContainerWindow(mDragAndDrop);
        trackWindow(mContainerWindow, "container");
        mHud = new HUD(mCustomMarkers, Settings::Manager::getInt("fps", "HUD"), mDragAndDrop);
        mToolTips = new ToolTips();
        mScrollWindow = new ScrollWindow();
        mBookWindow = new BookWindow();
        mCountDialog = new CountDialog();
        mSettingsWindow = new SettingsWindow();
        mConfirmationDialog = new ConfirmationDialog();
        mAlchemyWindow = new AlchemyWindow();
        trackWindow(mAlchemyWindow, "alchemy");
        mSpellWindow = new SpellWindow(mDragAndDrop);
        trackWindow(mSpellWindow, "spells");
        mQuickKeysMenu = new QuickKeysMenu();
        mLevelupDialog = new LevelupDialog();
        mWaitDialog = new WaitDialog();
        mSpellCreationDialog = new SpellCreationDialog();
        mEnchantingDialog = new EnchantingDialog();
        mTrainingWindow = new TrainingWindow();
        mMerchantRepair = new MerchantRepair();
        mRepair = new Repair();
        mSoulgemDialog = new SoulgemDialog(mMessageBoxManager);
        mCompanionWindow = new CompanionWindow(mDragAndDrop, mMessageBoxManager);
        trackWindow(mCompanionWindow, "companion");
        mJailScreen = new JailScreen();

        mWerewolfFader = new ScreenFader("textures\\werewolfoverlay.dds");
        mBlindnessFader = new ScreenFader("black.png");
        std::string hitFaderTexture = "textures\\bm_player_hit_01.dds";
        // fall back to player_hit_01.dds if bm_player_hit_01.dds is not available
        // TODO: check if non-BM versions actually use player_hit_01.dds
        if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(hitFaderTexture))
            hitFaderTexture = "textures\\player_hit_01.dds";
        mHitFader = new ScreenFader(hitFaderTexture);
        mScreenFader = new ScreenFader("black.png");

        mDebugWindow = new DebugWindow();

        mInputBlocker = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("",0,0,w,h,MyGUI::Align::Stretch,"Overlay");

        mHud->setVisible(mHudEnabled);

        mCharGen = new CharacterCreation();

        // Setup player stats
        for (int i = 0; i < ESM::Attribute::Length; ++i)
        {
            mPlayerAttributes.insert(std::make_pair(ESM::Attribute::sAttributeIds[i], MWMechanics::AttributeValue()));
        }

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            mPlayerSkillValues.insert(std::make_pair(ESM::Skill::sSkillIds[i], MWMechanics::SkillValue()));
        }

        // Set up visibility
        updateVisible();

        MWBase::Environment::get().getInputManager()->changeInputMode(false);
    }

    void WindowManager::renderWorldMap()
    {
        mMap->renderGlobalMap(mLoadingScreen);
    }

    void WindowManager::setNewGame(bool newgame)
    {
        // This method will always be called after loading a savegame or starting a new game
        // Reset enemy, it could be a dangling pointer from a previous game
        mHud->resetEnemy();

        if (newgame)
        {
            disallowAll();
            delete mCharGen;
            mCharGen = new CharacterCreation();
            mGuiModes.clear();
            MWBase::Environment::get().getInputManager()->changeInputMode(false);
            mHud->unsetSelectedWeapon();
            mHud->unsetSelectedSpell();
            unsetForceHide(GW_ALL);
        }
        else
            allow(GW_ALL);

        mRestAllowed = !newgame;
    }

    WindowManager::~WindowManager()
    {
        MyGUI::LanguageManager::getInstance().eventRequestTag.clear();
        MyGUI::PointerManager::getInstance().eventChangeMousePointer.clear();
        MyGUI::InputManager::getInstance().eventChangeKeyFocus.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

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
        delete mMerchantRepair;
        delete mRepair;
        delete mSoulgemDialog;
        delete mRecharge;
        delete mCompanionWindow;
        delete mHitFader;
        delete mWerewolfFader;
        delete mScreenFader;
        delete mBlindnessFader;
        delete mDebugWindow;
        delete mJailScreen;

        delete mCursorManager;

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

        mHud->update();
    }

    void WindowManager::updateVisible()
    {
        if (!mMap)
            return; // UI not created yet
        // Start out by hiding everything except the HUD
        mMap->setVisible(false);
        mMenu->setVisible(false);
        mStatsWindow->setVisible(false);
        mConsole->setVisible(false);
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
        mMerchantRepair->setVisible(false);
        mRepair->setVisible(false);
        mCompanionWindow->setVisible(false);
        mInventoryWindow->setTrading(false);
        mRecharge->setVisible(false);
        mVideoBackground->setVisible(false);
        mJailScreen->setVisible(false);

        mHud->setVisible(mHudEnabled && mGuiEnabled);
        mToolTips->setVisible(mGuiEnabled);

        bool gameMode = !isGuiMode();

        mInputBlocker->setVisible (gameMode);
        setCursorVisible(!gameMode);

        if (gameMode)
            setKeyFocusWidget (NULL);

        if (!mGuiEnabled)
        {
            if (containsMode(GM_Console))
                mConsole->setVisible(true);
            return;
        }

        // Icons of forced hidden windows are displayed
        setMinimapVisibility((mAllowed & GW_Map) && (!mMap->pinned() || (mForceHidden & GW_Map)));
        setWeaponVisibility((mAllowed & GW_Inventory) && (!mInventoryWindow->pinned() || (mForceHidden & GW_Inventory)));
        setSpellVisibility((mAllowed & GW_Magic) && (!mSpellWindow->pinned() || (mForceHidden & GW_Magic)));
        setHMSVisibility((mAllowed & GW_Stats) && (!mStatsWindow->pinned() || (mForceHidden & GW_Stats)));

        // If in game mode, show only the pinned windows
        if (gameMode)
        {
            mInventoryWindow->setGuiMode(GM_None);
            mMap->setVisible(mMap->pinned() && !(mForceHidden & GW_Map));
            mStatsWindow->setVisible(mStatsWindow->pinned() && !(mForceHidden & GW_Stats));
            mInventoryWindow->setVisible(mInventoryWindow->pinned() && !(mForceHidden & GW_Inventory));
            mSpellWindow->setVisible(mSpellWindow->pinned() && !(mForceHidden & GW_Magic));

            return;
        }

        if(mGuiModes.size() != 0)
        {
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
                    mConsole->setVisible(true);
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
                    int eff = mShown & mAllowed & ~mForceHidden;

                    // Show the windows we want
                    mMap            ->setVisible((eff & GW_Map) != 0);
                    mStatsWindow    ->setVisible((eff & GW_Stats) != 0);
                    mInventoryWindow->setVisible((eff & GW_Inventory) != 0);
                    mInventoryWindow->setGuiMode(mode);
                    mSpellWindow    ->setVisible((eff & GW_Magic) != 0);
                    break;
                }
                case GM_Container:
                    mContainerWindow->setVisible(true);
                    mInventoryWindow->setVisible(true);
                    mInventoryWindow->setGuiMode(mode);
                    break;
                case GM_Companion:
                    mCompanionWindow->setVisible(true);
                    mInventoryWindow->setVisible(true);
                    mInventoryWindow->setGuiMode(mode);
                    break;
                case GM_Dialogue:
                    mDialogueWindow->setVisible(true);
                    break;
                case GM_Barter:
                    mInventoryWindow->setVisible(true);
                    mInventoryWindow->setTrading(true);
                    mInventoryWindow->setGuiMode(mode);
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
                case GM_Recharge:
                    mRecharge->setVisible(true);
                    break;
                case GM_Enchanting:
                    mEnchantingDialog->setVisible(true);
                    break;
                case GM_Training:
                    mTrainingWindow->setVisible(true);
                    break;
                case GM_MerchantRepair:
                    mMerchantRepair->setVisible(true);
                    break;
                case GM_Repair:
                    mRepair->setVisible(true);
                    break;
                case GM_Journal:
                    mJournal->setVisible(true);
                    break;
                case GM_Jail:
                    mJailScreen->setVisible(true);
                    break;
                case GM_LoadingWallpaper:
                case GM_Loading:
                    // Don't need to show anything here - GM_LoadingWallpaper covers everything else anyway,
                    // GM_Loading uses a texture of the last rendered frame so everything previously visible will be rendered.
                    mHud->setVisible(false);
                    mToolTips->setVisible(false);
                    setCursorVisible(mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox());
                    break;
                default:
                    // Unsupported mode, switch back to game
                    break;
            }
        }
    }

    void WindowManager::setValue (const std::string& id, const MWMechanics::AttributeValue& value)
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


    void WindowManager::setValue (int parSkill, const MWMechanics::SkillValue& value)
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
    }

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

    void WindowManager::setDrowningTimeLeft (float time, float maxTime)
    {
        mHud->setDrowningTimeLeft(time, maxTime);
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

    void WindowManager::exitCurrentGuiMode() {
        switch(mGuiModes.back()) {
            case GM_QuickKeysMenu:
                mQuickKeysMenu->exit();
                break;
            case GM_MainMenu:
                removeGuiMode(GM_MainMenu); //Simple way to remove it
                break;
            case GM_Settings:
                mSettingsWindow->exit();
                break;
            case GM_Console:
                mConsole->exit();
                break;
            case GM_Scroll:
                mScrollWindow->exit();
                break;
            case GM_Book:
                mBookWindow->exit();
                break;
            case GM_Alchemy:
                mAlchemyWindow->exit();
                break;
            case GM_Rest:
                mWaitDialog->exit();
                break;
            case GM_RestBed:
                mWaitDialog->exit();
                break;
            case GM_Name:
            case GM_Race:
            case GM_Class:
            case GM_ClassPick:
            case GM_ClassCreate:
            case GM_Birth:
            case GM_ClassGenerate:
            case GM_Review:
                break;
            case GM_Inventory:
                removeGuiMode(GM_Inventory); //Simple way to remove it
                break;
            case GM_Container:
                mContainerWindow->exit();
                break;
            case GM_Companion:
                mCompanionWindow->exit();
                break;
            case GM_Dialogue:
                mDialogueWindow->exit();
                break;
            case GM_Barter:
                mTradeWindow->exit();
                break;
            case GM_SpellBuying:
                mSpellBuyingWindow->exit();
                break;
            case GM_Travel:
                mTravelWindow->exit();
                break;
            case GM_SpellCreation:
                mSpellCreationDialog->exit();
                break;
            case GM_Recharge:
                mRecharge->exit();
                break;
            case GM_Enchanting:
                mEnchantingDialog->exit();
                break;
            case GM_Training:
                mTrainingWindow->exit();
                break;
            case GM_MerchantRepair:
                mMerchantRepair->exit();
                break;
            case GM_Repair:
                mRepair->exit();
                break;
            case GM_Journal:
                MWBase::Environment::get().getSoundManager()->playSound ("book close", 1.0, 1.0);
                removeGuiMode(GM_Journal); //Simple way to remove it
                break;
            default:
                // Unsupported mode, switch back to game
                break;
        }
    }

    void WindowManager::interactiveMessageBox(const std::string &message, const std::vector<std::string> &buttons, bool block)
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons);
        MWBase::Environment::get().getInputManager()->changeInputMode(isGuiMode());
        updateVisible();

        if (block)
        {
            while (mMessageBoxManager->readPressedButton(false) == -1
                   && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
            {
                mMessageBoxManager->onFrame(0.f);
                MWBase::Environment::get().getInputManager()->update(0, true, false);

                mRendering->getWindow()->update();
            }
        }
    }

    void WindowManager::messageBox (const std::string& message, enum MWGui::ShowInDialogueMode showInDialogueMode)
    {
        if (getMode() == GM_Dialogue && showInDialogueMode != MWGui::ShowInDialogueMode_Never) {
            mDialogueWindow->addMessageBox(MyGUI::LanguageManager::getInstance().replaceTags(message));
        } else if (showInDialogueMode != MWGui::ShowInDialogueMode_Only) {
            mMessageBoxManager->createMessageBox(message);
        }
    }

    void WindowManager::staticMessageBox(const std::string& message)
    {
        mMessageBoxManager->createMessageBox(message, true);
    }

    void WindowManager::removeStaticMessageBox()
    {
        mMessageBoxManager->removeStaticMessageBox();
    }

    int WindowManager::readPressedButton ()
    {
        return mMessageBoxManager->readPressedButton();
    }

    std::string WindowManager::getGameSettingString(const std::string &id, const std::string &default_)
    {
        const ESM::GameSetting *setting =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().search(id);

        if (setting && setting->mValue.getType()==ESM::VT_String)
            return setting->mValue.getString();

        return default_;
    }

    void WindowManager::onFrame (float frameDuration)
    {
        mMessageBoxManager->onFrame(frameDuration);

        mToolTips->onFrame(frameDuration);

        mMenu->update(frameDuration);

        if (MWBase::Environment::get().getStateManager()->getState()==
            MWBase::StateManager::State_NoGame)
            return;

        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            assert(mDragAndDrop->mDraggedWidget);
            mDragAndDrop->mDraggedWidget->setPosition(MyGUI::InputManager::getInstance().getMousePosition());
        }

        mDialogueWindow->onFrame();

        mInventoryWindow->onFrame();

        mStatsWindow->onFrame(frameDuration);
        mMap->onFrame(frameDuration);
        mSpellWindow->onFrame(frameDuration);

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
        mCompanionWindow->checkReferenceAvailable();
        mConsole->checkReferenceAvailable();
        mCompanionWindow->onFrame();
        mJailScreen->onFrame(frameDuration);

        mWerewolfFader->update(frameDuration);
        mBlindnessFader->update(frameDuration);
        mHitFader->update(frameDuration);
        mScreenFader->update(frameDuration);

        mDebugWindow->onFrame(frameDuration);
    }

    void WindowManager::changeCell(MWWorld::CellStore* cell)
    {
        std::string name = MWBase::Environment::get().getWorld()->getCellName (cell);

        mMap->setCellName( name );
        mHud->setCellName( name );

        if (cell->getCell()->isExterior())
        {
            if (!cell->getCell()->mName.empty())
                mMap->addVisitedLocation ("#{sCell=" + name + "}", cell->getCell()->getGridX (), cell->getCell()->getGridY ());

            mMap->cellExplored (cell->getCell()->getGridX(), cell->getCell()->getGridY());
        }
        else
        {
            mMap->setCellPrefix (cell->getCell()->mName );
            mHud->setCellPrefix (cell->getCell()->mName );

            Ogre::Vector3 worldPos;
            if (!MWBase::Environment::get().getWorld()->findInteriorPositionInWorldSpace(cell, worldPos))
                worldPos = MWBase::Environment::get().getWorld()->getPlayer().getLastKnownExteriorPosition();
            else
                MWBase::Environment::get().getWorld()->getPlayer().setLastKnownExteriorPosition(worldPos);
            mMap->setGlobalMapPlayerPosition(worldPos.x, worldPos.y);
        }
    }

    void WindowManager::setActiveMap(int x, int y, bool interior)
    {
        if (!interior)
        {
            mMap->setCellPrefix("Cell");
            mHud->setCellPrefix("Cell");
        }

        mMap->setActiveCell(x,y, interior);
        mHud->setActiveCell(x,y, interior);
    }

    void WindowManager::setPlayerPos(int cellX, int cellY, const float x, const float y)
    {
        mMap->setPlayerPos(cellX, cellY, x, y);
        mHud->setPlayerPos(cellX, cellY, x, y);
    }

    void WindowManager::setPlayerDir(const float x, const float y)
    {
        mMap->setPlayerDir(x,y);
        mMap->setGlobalMapPlayerDir(x, y);
        mHud->setPlayerDir(x,y);
    }

    void WindowManager::setDrowningBarVisibility(bool visible)
    {
        mHud->setDrowningBarVisible(visible);
    }

    void WindowManager::setHMSVisibility(bool visible)
    {
        mHud->setHmsVisible (visible);
    }

    void WindowManager::setMinimapVisibility(bool visible)
    {
        mHud->setMinimapVisible (visible);
    }

    bool WindowManager::toggleFogOfWar()
    {
        mMap->toggleFogOfWar();
        return mHud->toggleFogOfWar();
    }

    void WindowManager::setFocusObject(const MWWorld::Ptr& focus)
    {
        mToolTips->setFocusObject(focus);
    }

    void WindowManager::setFocusObjectScreenCoords(float min_x, float min_y, float max_x, float max_y)
    {
        mToolTips->setFocusObjectScreenCoords(min_x, min_y, max_x, max_y);
    }

    bool WindowManager::toggleFullHelp()
    {
        return mToolTips->toggleFullHelp();
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

    void WindowManager::setSneakVisibility(bool visible)
    {
        mHud->setSneakVisible(visible);
    }

    void WindowManager::setDragDrop(bool dragDrop)
    {
        mToolTips->setEnabled(!dragDrop);
        MWBase::Environment::get().getInputManager()->setDragDrop(dragDrop);
    }

    void WindowManager::setCursorVisible(bool visible)
    {
        mCursorVisible = visible;
    }

    void WindowManager::onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
    {
        std::string tag(_tag);

        std::string tokenToFind = "sCell=";
        size_t tokenLength = tokenToFind.length();

        if (tag.compare(0, tokenLength, tokenToFind) == 0)
        {
            _result = mTranslationDataStorage.translateCellName(tag.substr(tokenLength));
        }
        else if (Gui::replaceTag(tag, _result, mFallbackMap))
        {
            return;
        }
        else
        {
            const ESM::GameSetting *setting =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(tag);

            if (setting && setting->mValue.getType()==ESM::VT_String)
                _result = setting->mValue.getString();
            else
                _result = tag;
        }
    }

    void WindowManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        mHud->setFpsLevel(Settings::Manager::getInt("fps", "HUD"));
        mToolTips->setDelay(Settings::Manager::getFloat("tooltip delay", "GUI"));

        for (Settings::CategorySettingVector::const_iterator it = changed.begin();
            it != changed.end(); ++it)
        {
            if (it->first == "HUD" && it->second == "crosshair")
                mCrosshairEnabled = Settings::Manager::getBool ("crosshair", "HUD");
            else if (it->first == "GUI" && it->second == "subtitles")
                mSubtitlesEnabled = Settings::Manager::getBool ("subtitles", "GUI");
        }
    }

    void WindowManager::windowResized(int x, int y)
    {
        sizeVideo(x, y);
        mGuiManager->windowResized();
        if (!mHud)
            return; // UI not initialized yet

        for (std::map<MyGUI::Window*, std::string>::iterator it = mTrackedWindows.begin(); it != mTrackedWindows.end(); ++it)
        {
            MyGUI::IntPoint pos(static_cast<int>(Settings::Manager::getFloat(it->second + " x", "Windows") * x),
                                static_cast<int>( Settings::Manager::getFloat(it->second+ " y", "Windows") * y));
            MyGUI::IntSize size(static_cast<int>(Settings::Manager::getFloat(it->second + " w", "Windows") * x),
                                 static_cast<int>(Settings::Manager::getFloat(it->second + " h", "Windows") * y));
            it->first->setPosition(pos);
            it->first->setSize(size);
        }

        mConsole->onResChange(x, y);
        mMenu->onResChange(x, y);
        mSettingsWindow->center();
        mAlchemyWindow->center();
        mScrollWindow->center();
        mBookWindow->center();
        mQuickKeysMenu->center();
        mSpellBuyingWindow->center();
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

    void WindowManager::onCursorChange(const std::string &name)
    {
        if(!mCursorManager->cursorChanged(name))
            return; //the cursor manager doesn't want any more info about this cursor
        //See if we can get the information we need out of the cursor resource
        ResourceImageSetPointerFix* imgSetPtr = dynamic_cast<ResourceImageSetPointerFix*>(MyGUI::PointerManager::getInstance().getByName(name));
        if(imgSetPtr != NULL)
        {
            MyGUI::ResourceImageSet* imgSet = imgSetPtr->getImageSet();

            std::string tex_name = imgSet->getIndexInfo(0,0).texture;

            Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName(tex_name);

            //everything looks good, send it to the cursor manager
            if(!tex.isNull())
            {
                Uint8 size_x = imgSetPtr->getSize().width;
                Uint8 size_y = imgSetPtr->getSize().height;
                Uint8 hotspot_x = imgSetPtr->getHotSpot().left;
                Uint8 hotspot_y = imgSetPtr->getHotSpot().top;
                int rotation = imgSetPtr->getRotation();

                mCursorManager->receiveCursorInfo(name, rotation, tex, size_x, size_y, hotspot_x, hotspot_y);
            }
        }
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

    void WindowManager::goToJail(int days)
    {
        pushGuiMode(MWGui::GM_Jail);
        mJailScreen->goToJail(days);
    }

    void WindowManager::setSelectedSpell(const std::string& spellId, int successChancePercent)
    {
        mSelectedSpell = spellId;
        mHud->setSelectedSpell(spellId, successChancePercent);

        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        mSpellWindow->setTitle(spell->mName);
    }

    void WindowManager::setSelectedEnchantItem(const MWWorld::Ptr& item)
    {
        mSelectedSpell = "";
        const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>()
                .find(item.getClass().getEnchantment(item));

        int chargePercent = (item.getCellRef().getEnchantmentCharge() == -1) ? 100
                : static_cast<int>(item.getCellRef().getEnchantmentCharge() / static_cast<float>(ench->mData.mCharge) * 100);
        mHud->setSelectedEnchantItem(item, chargePercent);
        mSpellWindow->setTitle(item.getClass().getName(item));
    }

    void WindowManager::setSelectedWeapon(const MWWorld::Ptr& item)
    {
        int durabilityPercent =
             static_cast<int>(item.getClass().getItemHealth(item) / static_cast<float>(item.getClass().getItemMaxHealth(item)) * 100);
        mHud->setSelectedWeapon(item, durabilityPercent);
        mInventoryWindow->setTitle(item.getClass().getName(item));
    }

    void WindowManager::unsetSelectedSpell()
    {
        mSelectedSpell = "";
        mHud->unsetSelectedSpell();

        MWWorld::Player* player = &MWBase::Environment::get().getWorld()->getPlayer();
        if (player->getDrawState() == MWMechanics::DrawState_Spell)
            player->setDrawState(MWMechanics::DrawState_Nothing);

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
        x = static_cast<float>(pos.left);
        y = static_cast<float>(pos.top);
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

    MWGui::DialogueWindow* WindowManager::getDialogueWindow() { return mDialogueWindow;  }
    MWGui::InventoryWindow* WindowManager::getInventoryWindow() { return mInventoryWindow; }
    MWGui::CountDialog* WindowManager::getCountDialog() { return mCountDialog; }
    MWGui::ConfirmationDialog* WindowManager::getConfirmationDialog() { return mConfirmationDialog; }
    MWGui::TradeWindow* WindowManager::getTradeWindow() { return mTradeWindow; }

    bool WindowManager::isAllowed (GuiWindow wnd) const
    {
        return (mAllowed & wnd) != 0;
    }

    void WindowManager::allow (GuiWindow wnd)
    {
        mAllowed = (GuiWindow)(mAllowed | wnd);

        if (wnd & GW_Inventory)
        {
            mBookWindow->setInventoryAllowed (true);
            mScrollWindow->setInventoryAllowed (true);
        }

        updateVisible();
    }

    void WindowManager::disallowAll()
    {
        mAllowed = GW_None;

        mBookWindow->setInventoryAllowed (false);
        mScrollWindow->setInventoryAllowed (false);

        updateVisible();
    }

    void WindowManager::toggleVisible (GuiWindow wnd)
    {
        if (getMode() != GM_Inventory)
            return;

        mShown = (GuiWindow)(mShown ^ wnd);
        updateVisible();
    }

    void WindowManager::forceHide(GuiWindow wnd)
    {
        mForceHidden = (GuiWindow)(mForceHidden | wnd);
        updateVisible();
    }

    void WindowManager::unsetForceHide(GuiWindow wnd)
    {
        mForceHidden = (GuiWindow)(mForceHidden & ~wnd);
        updateVisible();
    }

    bool WindowManager::isGuiMode() const
    {
        return !mGuiModes.empty() || (mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox());
    }

    bool WindowManager::isConsoleMode() const
    {
        if (!mGuiModes.empty() && mGuiModes.back()==GM_Console)
            return true;
        return false;
    }

    MWGui::GuiMode WindowManager::getMode() const
    {
        if (mGuiModes.empty())
            return GM_None;
        return mGuiModes.back();
    }

    std::map<int, MWMechanics::SkillValue > WindowManager::getPlayerSkillValues()
    {
        return mPlayerSkillValues;
    }

    std::map<int, MWMechanics::AttributeValue > WindowManager::getPlayerAttributeValues()
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

    bool WindowManager::containsMode(GuiMode mode) const
    {
        if(mGuiModes.empty())
            return false;

        return std::find(mGuiModes.begin(), mGuiModes.end(), mode) != mGuiModes.end();
    }

    void WindowManager::showCrosshair (bool show)
    {
        if (mHud)
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

    bool WindowManager::toggleGui()
    {
        mGuiEnabled = !mGuiEnabled;
        updateVisible();
        return mGuiEnabled;
    }

    bool WindowManager::getRestEnabled()
    {
        //Enable rest dialogue if character creation finished
        if(mRestAllowed==false && MWBase::Environment::get().getWorld()->getGlobalFloat ("chargenstate")==-1)
            mRestAllowed=true;
        return mRestAllowed;
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
        pushGuiMode(GM_SpellCreation);
        mSpellCreationDialog->startSpellMaking (actor);
    }

    void WindowManager::startEnchanting (MWWorld::Ptr actor)
    {
        pushGuiMode(GM_Enchanting);
        mEnchantingDialog->startEnchanting (actor);
    }

    void WindowManager::startSelfEnchanting(MWWorld::Ptr soulgem)
    {
        mEnchantingDialog->startSelfEnchanting(soulgem);
    }

    void WindowManager::startTraining(MWWorld::Ptr actor)
    {
        pushGuiMode(GM_Training);
        mTrainingWindow->startTraining(actor);
    }

    void WindowManager::startRepair(MWWorld::Ptr actor)
    {
        pushGuiMode(GM_MerchantRepair);
        mMerchantRepair->startRepair(actor);
    }

    void WindowManager::startRepairItem(MWWorld::Ptr item)
    {
        pushGuiMode(MWGui::GM_Repair);
        mRepair->startRepairItem(item);
    }

    const Translation::Storage& WindowManager::getTranslationDataStorage() const
    {
        return mTranslationDataStorage;
    }

    void WindowManager::showCompanionWindow(MWWorld::Ptr actor)
    {
        pushGuiMode(MWGui::GM_Companion);
        mCompanionWindow->open(actor);
    }

    void WindowManager::changePointer(const std::string &name)
    {
        MyGUI::PointerManager::getInstance().setPointer(name);
        onCursorChange(name);
    }

    void WindowManager::showSoulgemDialog(MWWorld::Ptr item)
    {
        mSoulgemDialog->show(item);
        MWBase::Environment::get().getInputManager()->changeInputMode(isGuiMode());
        updateVisible();
    }

    void WindowManager::frameStarted (float dt)
    {
        mInventoryWindow->doRenderUpdate ();
        mCharGen->doRenderUpdate();
    }

    void WindowManager::updatePlayer()
    {
        mInventoryWindow->updatePlayer();

        const MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            setWerewolfOverlay(true);
            forceHide((GuiWindow)(MWGui::GW_Inventory | MWGui::GW_Magic));
        }
    }

    // Remove this method for MyGUI 3.2.2
    void WindowManager::setKeyFocusWidget(MyGUI::Widget *widget)
    {
        if (widget == NULL)
            MyGUI::InputManager::getInstance().resetKeyFocusWidget();
        else
            MyGUI::InputManager::getInstance().setKeyFocusWidget(widget);
        onKeyFocusChanged(widget);
    }

    void WindowManager::onKeyFocusChanged(MyGUI::Widget *widget)
    {
        if (widget && widget->castType<MyGUI::EditBox>(false))
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }

    void WindowManager::setEnemy(const MWWorld::Ptr &enemy)
    {
        mHud->setEnemy(enemy);
    }

    Loading::Listener* WindowManager::getLoadingScreen()
    {
        return mLoadingScreen;
    }

    void WindowManager::startRecharge(MWWorld::Ptr soulgem)
    {
        mRecharge->start(soulgem);
    }

    bool WindowManager::getCursorVisible()
    {
        return mCursorVisible;
    }

    void WindowManager::trackWindow(OEngine::GUI::Layout *layout, const std::string &name)
    {
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos(static_cast<int>(Settings::Manager::getFloat(name + " x", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(name + " y", "Windows") * viewSize.height));
        MyGUI::IntSize size (static_cast<int>(Settings::Manager::getFloat(name + " w", "Windows") * viewSize.width),
                             static_cast<int>(Settings::Manager::getFloat(name + " h", "Windows") * viewSize.height));
        layout->mMainWidget->setPosition(pos);
        layout->mMainWidget->setSize(size);

        MyGUI::Window* window = dynamic_cast<MyGUI::Window*>(layout->mMainWidget);
        if (!window)
            throw std::runtime_error("Attempting to track size of a non-resizable window");
        window->eventWindowChangeCoord += MyGUI::newDelegate(this, &WindowManager::onWindowChangeCoord);
        mTrackedWindows[window] = name;
    }

    void WindowManager::onWindowChangeCoord(MyGUI::Window *_sender)
    {
        std::string setting = mTrackedWindows[_sender];
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        float x = _sender->getPosition().left / float(viewSize.width);
        float y = _sender->getPosition().top / float(viewSize.height);
        float w = _sender->getSize().width / float(viewSize.width);
        float h = _sender->getSize().height / float(viewSize.height);
        Settings::Manager::setFloat(setting + " x", "Windows", x);
        Settings::Manager::setFloat(setting + " y", "Windows", y);
        Settings::Manager::setFloat(setting + " w", "Windows", w);
        Settings::Manager::setFloat(setting + " h", "Windows", h);
    }

    void WindowManager::clear()
    {
        mMap->clear();
        mQuickKeysMenu->clear();
        mMessageBoxManager->clear();

        mTrainingWindow->resetReference();
        mDialogueWindow->resetReference();
        mTradeWindow->resetReference();
        mSpellBuyingWindow->resetReference();
        mSpellCreationDialog->resetReference();
        mEnchantingDialog->resetReference();
        mContainerWindow->resetReference();
        mCompanionWindow->resetReference();
        mConsole->resetReference();

        mSelectedSpell.clear();

        mCustomMarkers.clear();

        mForceHidden = GW_None;

        setWerewolfOverlay(false);

        mGuiModes.clear();
        MWBase::Environment::get().getInputManager()->changeInputMode(false);
        updateVisible();
    }

    void WindowManager::write(ESM::ESMWriter &writer, Loading::Listener& progress)
    {
        mMap->write(writer, progress);

        mQuickKeysMenu->write(writer);

        if (!mSelectedSpell.empty())
        {
            writer.startRecord(ESM::REC_ASPL);
            writer.writeHNString("ID__", mSelectedSpell);
            writer.endRecord(ESM::REC_ASPL);
        }

        for (std::vector<ESM::CustomMarker>::const_iterator it = mCustomMarkers.begin(); it != mCustomMarkers.end(); ++it)
        {
            writer.startRecord(ESM::REC_MARK);
            (*it).save(writer);
            writer.endRecord(ESM::REC_MARK);
        }
    }

    void WindowManager::readRecord(ESM::ESMReader &reader, uint32_t type)
    {
        if (type == ESM::REC_GMAP)
            mMap->readRecord(reader, type);
        else if (type == ESM::REC_KEYS)
            mQuickKeysMenu->readRecord(reader, type);
        else if (type == ESM::REC_ASPL)
        {
            reader.getSubNameIs("ID__");
            mSelectedSpell = reader.getHString();
        }
        else if (type == ESM::REC_MARK)
        {
            ESM::CustomMarker marker;
            marker.load(reader);
            mCustomMarkers.addMarker(marker, false);
        }
    }

    int WindowManager::countSavedGameRecords() const
    {
        return 1 // Global map
                + 1 // QuickKeysMenu
                + mCustomMarkers.size()
                + (!mSelectedSpell.empty() ? 1 : 0);
    }

    bool WindowManager::isSavingAllowed() const
    {
        return !MyGUI::InputManager::getInstance().isModalAny()
                // TODO: remove this, once we have properly serialized the state of open windows
                && (!isGuiMode() || (mGuiModes.size() == 1 && (getMode() == GM_MainMenu || getMode() == GM_Rest || getMode() == GM_RestBed)));
    }

    void WindowManager::playVideo(const std::string &name, bool allowSkipping)
    {
        mVideoWidget->playVideo("video\\" + name);

        mVideoWidget->eventKeyButtonPressed.clear();
        mVideoBackground->eventKeyButtonPressed.clear();
        if (allowSkipping)
        {
            mVideoWidget->eventKeyButtonPressed += MyGUI::newDelegate(this, &WindowManager::onVideoKeyPressed);
            mVideoBackground->eventKeyButtonPressed += MyGUI::newDelegate(this, &WindowManager::onVideoKeyPressed);
        }

        // Turn off all rendering except for the GUI
        mRendering->getScene()->clearSpecialCaseRenderQueues();
        // SCRQM_INCLUDE with RENDER_QUEUE_OVERLAY does not work?
        for(int i = 0;i < Ogre::RENDER_QUEUE_MAX;++i)
        {
            if(i > 0 && i < 96)
                mRendering->getScene()->addSpecialCaseRenderQueue(i);
        }
        mRendering->getScene()->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

        MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
        sizeVideo(screenSize.width, screenSize.height);

        setKeyFocusWidget(mVideoWidget);

        mVideoBackground->setVisible(true);

        bool cursorWasVisible = mCursorVisible;
        setCursorVisible(false);

        if (mVideoWidget->hasAudioStream())
            MWBase::Environment::get().getSoundManager()->pauseSounds(
                        MWBase::SoundManager::Play_TypeMask&(~MWBase::SoundManager::Play_TypeMovie));

        while (mVideoWidget->update() && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
        {
            MWBase::Environment::get().getInputManager()->update(0, true, false);

            mRendering->getWindow()->update();
        }
        mVideoWidget->stop();

        MWBase::Environment::get().getSoundManager()->resumeSounds();

        setCursorVisible(cursorWasVisible);

        // Restore normal rendering
        mRendering->getScene()->clearSpecialCaseRenderQueues();
        mRendering->getScene()->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

        mVideoBackground->setVisible(false);
    }

    void WindowManager::sizeVideo(int screenWidth, int screenHeight)
    {
        // Use black bars to correct aspect ratio
        bool stretch = Settings::Manager::getBool("stretch menu background", "GUI");
        mVideoBackground->setSize(screenWidth, screenHeight);
        mVideoWidget->autoResize(stretch);
    }

    void WindowManager::exitCurrentModal()
    {
        if (!mCurrentModals.empty())
            mCurrentModals.top()->exit();
    }

    void WindowManager::removeCurrentModal(WindowModal* input)
    {
        // Only remove the top if it matches the current pointer. A lot of things hide their visibility before showing it,
        //so just popping the top would cause massive issues.
        if(!mCurrentModals.empty())
            if(input == mCurrentModals.top())
                mCurrentModals.pop();
    }

    void WindowManager::onVideoKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
    {
        if (_key == MyGUI::KeyCode::Escape)
            mVideoWidget->stop();
    }

    void WindowManager::pinWindow(GuiWindow window)
    {
        switch (window)
        {
        case GW_Inventory:
            mInventoryWindow->setPinned(true);
            break;
        case GW_Map:
            mMap->setPinned(true);
            break;
        case GW_Magic:
            mSpellWindow->setPinned(true);
            break;
        case GW_Stats:
            mStatsWindow->setPinned(true);
            break;
        default:
            break;
        }

        updateVisible();
    }

    void WindowManager::fadeScreenIn(const float time, bool clearQueue)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeOut(time);
    }

    void WindowManager::fadeScreenOut(const float time, bool clearQueue)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeIn(time);
    }

    void WindowManager::fadeScreenTo(const int percent, const float time, bool clearQueue)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeTo(percent, time);
    }

    void WindowManager::setBlindness(const int percent)
    {
        mBlindnessFader->notifyAlphaChanged(percent / 100.f);
    }

    void WindowManager::activateHitOverlay(bool interrupt)
    {
        if (!mHitFaderEnabled)
            return;

        if (!interrupt && !mHitFader->isEmpty())
            return;

        mHitFader->clearQueue();
        mHitFader->fadeTo(100, 0.0f);
        mHitFader->fadeTo(0, 0.5f);
    }

    void WindowManager::setWerewolfOverlay(bool set)
    {
        if (!mWerewolfOverlayEnabled)
            return;

        mWerewolfFader->notifyAlphaChanged(set ? 1.0f : 0.0f);
    }

    void WindowManager::onClipboardChanged(const std::string &_type, const std::string &_data)
    {
        if (_type == "Text")
            SDL_SetClipboardText(MyGUI::TextIterator::getOnlyText(MyGUI::UString(_data)).asUTF8().c_str());
    }

    void WindowManager::onClipboardRequested(const std::string &_type, std::string &_data)
    {
        if (_type != "Text")
            return;
        char* text=0;
        text = SDL_GetClipboardText();
        if (text)
        {
            // MyGUI's clipboard might still have color information, to retain that information, only set the new text
            // if it actually changed (clipboard inserted by an external application)
            if (MyGUI::TextIterator::getOnlyText(_data) != text)
                _data = text;
        }
        SDL_free(text);
    }

    void WindowManager::toggleDebugWindow()
    {
        mDebugWindow->setVisible(!mDebugWindow->isVisible());
    }

    void WindowManager::cycleSpell(bool next)
    {
        mSpellWindow->cycle(next);
    }

    void WindowManager::cycleWeapon(bool next)
    {
        mInventoryWindow->cycle(next);
    }

    void WindowManager::setConsoleSelectedObject(const MWWorld::Ptr &object)
    {
        mConsole->setSelectedObject(object);
    }

    void WindowManager::updateSpellWindow()
    {
        if (mSpellWindow)
            mSpellWindow->updateSpells();
    }

    void WindowManager::startTravel(const MWWorld::Ptr &actor)
    {
        pushGuiMode(GM_Travel);
        mTravelWindow->startTravel(actor);
    }

    void WindowManager::startSpellBuying(const MWWorld::Ptr &actor)
    {
        pushGuiMode(GM_SpellBuying);
        mSpellBuyingWindow->startSpellBuying(actor);
    }

    void WindowManager::startTrade(const MWWorld::Ptr &actor)
    {
        pushGuiMode(GM_Barter);
        mTradeWindow->startTrade(actor);
    }

    void WindowManager::openContainer(const MWWorld::Ptr &container, bool loot)
    {
        pushGuiMode(GM_Container);
        mContainerWindow->open(container, loot);
    }

    void WindowManager::showBook(const MWWorld::Ptr &item, bool showTakeButton)
    {
        pushGuiMode(GM_Book);
        mBookWindow->open(item, showTakeButton);
    }

    void WindowManager::showScroll(const MWWorld::Ptr &item, bool showTakeButton)
    {
        pushGuiMode(GM_Scroll);
        mScrollWindow->open(item, showTakeButton);
    }

}
