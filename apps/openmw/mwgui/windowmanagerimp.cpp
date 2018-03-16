#include "windowmanagerimp.hpp"

#include <cassert>
#include <iterator>

#include <osgViewer/Viewer>

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
#include <MyGUI_WidgetManager.h>

#include <SDL_keyboard.h>
#include <SDL_clipboard.h>

#include <components/sdlutil/sdlcursormanager.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>

#include <components/fontloader/fontloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>

#include <components/sceneutil/workqueue.hpp>

#include <components/translation/translation.hpp>

#include <components/myguiplatform/myguiplatform.hpp>
#include <components/myguiplatform/myguirendermanager.hpp>
#include <components/myguiplatform/additivelayer.hpp>
#include <components/myguiplatform/scalinglayer.hpp>

#include <components/vfs/manager.hpp>

#include <components/widgets/widgets.hpp>
#include <components/widgets/tags.hpp>

#include <components/sdlutil/sdlcursormanager.hpp>

#include <components/misc/resourcehelpers.hpp>

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwrender/vismask.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/stat.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwrender/localmap.hpp"

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
#include "itemchargeview.hpp"
#include "keyboardnavigation.hpp"

namespace
{

    MyGUI::Colour getTextColour(const std::string& type)
    {
        return MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=" + type + "}"));
    }

}

namespace MWGui
{

    WindowManager::WindowManager(
            osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue,
            const std::string& logpath, const std::string& resourcePath, bool consoleOnlyScripts,
            Translation::Storage& translationDataStorage, ToUTF8::FromType encoding, bool exportFonts, const std::map<std::string, std::string>& fallbackMap, const std::string& versionDescription)
      : mStore(NULL)
      , mResourceSystem(resourceSystem)
      , mWorkQueue(workQueue)
      , mViewer(viewer)
      , mConsoleOnlyScripts(consoleOnlyScripts)
      , mCurrentModals()
      , mHud(NULL)
      , mMap(NULL)
      , mLocalMapRender(NULL)
      , mToolTips(NULL)
      , mStatsWindow(NULL)
      , mMessageBoxManager(NULL)
      , mConsole(NULL)
      , mDialogueWindow(NULL)
      , mDragAndDrop(NULL)
      , mInventoryWindow(NULL)
      , mScrollWindow(NULL)
      , mBookWindow(NULL)
      , mCountDialog(NULL)
      , mTradeWindow(NULL)
      , mSettingsWindow(NULL)
      , mConfirmationDialog(NULL)
      , mSpellWindow(NULL)
      , mQuickKeysMenu(NULL)
      , mLoadingScreen(NULL)
      , mWaitDialog(NULL)
      , mSoulgemDialog(NULL)
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
      , mCursorVisible(true)
      , mCursorActive(false)
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
      , mFallbackMap(fallbackMap)
      , mShowOwned(0)
      , mEncoding(encoding)
      , mVersionDescription(versionDescription)
    {
        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        mGuiPlatform = new osgMyGUI::Platform(viewer, guiRoot, resourceSystem->getImageManager(), uiScale);
        mGuiPlatform->initialise(resourcePath, logpath);

        mGui = new MyGUI::Gui;
        mGui->initialise("");

        createTextures();

        MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &WindowManager::onRetrieveTag);

        // Load fonts
        mFontLoader.reset(new Gui::FontLoader(encoding, resourceSystem->getVFS()));
        mFontLoader->loadAllFonts(exportFonts);

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
        MyGUI::FactoryManager::getInstance().registerFactory<osgMyGUI::AdditiveLayer>("Layer");
        MyGUI::FactoryManager::getInstance().registerFactory<osgMyGUI::ScalingLayer>("Layer");
        BookPage::registerMyGUIComponents ();
        ItemView::registerComponents();
        ItemChargeView::registerComponents();
        ItemWidget::registerComponents();
        SpellView::registerComponents();
        Gui::registerAllWidgets();

        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Controllers::ControllerRepeatEvent>("Controller");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Controllers::ControllerFollowMouse>("Controller");

        MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>("Resource", "ResourceImageSetPointer");
        MyGUI::ResourceManager::getInstance().load("core.xml");

        bool keyboardNav = Settings::Manager::getBool("keyboard navigation", "GUI");
        mKeyboardNavigation.reset(new KeyboardNavigation());
        mKeyboardNavigation->setEnabled(keyboardNav);
        Gui::ImageButton::setDefaultNeedKeyFocus(keyboardNav);

        mLoadingScreen = new LoadingScreen(mResourceSystem->getVFS(), mViewer);
        mWindows.push_back(mLoadingScreen);

        //set up the hardware cursor manager
        mCursorManager = new SDLUtil::SDLCursorManager();

        MyGUI::PointerManager::getInstance().eventChangeMousePointer += MyGUI::newDelegate(this, &WindowManager::onCursorChange);

        MyGUI::InputManager::getInstance().eventChangeKeyFocus += MyGUI::newDelegate(this, &WindowManager::onKeyFocusChanged);

        // Create all cursors in advance
        createCursors();
        onCursorChange(MyGUI::PointerManager::getInstance().getDefaultPointer());
        mCursorManager->setEnabled(true);

        // hide mygui's pointer
        MyGUI::PointerManager::getInstance().setVisible(false);

        mVideoBackground = MyGUI::Gui::getInstance().createWidgetReal<MyGUI::ImageBox>("ImageBox", 0,0,1,1,
            MyGUI::Align::Default, "InputBlocker");
        mVideoBackground->setImageTexture("black");
        mVideoBackground->setVisible(false);
        mVideoBackground->setNeedMouseFocus(true);
        mVideoBackground->setNeedKeyFocus(true);

        mVideoWidget = mVideoBackground->createWidgetReal<VideoWidget>("ImageBox", 0,0,1,1, MyGUI::Align::Default);
        mVideoWidget->setNeedMouseFocus(true);
        mVideoWidget->setNeedKeyFocus(true);
        mVideoWidget->setVFS(resourceSystem->getVFS());

        // Removes default MyGUI system clipboard implementation, which supports windows only
        MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

        MyGUI::ClipboardManager::getInstance().eventClipboardChanged += MyGUI::newDelegate(this, &WindowManager::onClipboardChanged);
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested += MyGUI::newDelegate(this, &WindowManager::onClipboardRequested);

        mShowOwned = Settings::Manager::getInt("show owned", "Game");
    }

    void WindowManager::initUI()
    {
        // Get size info from the Gui object
        int w = MyGUI::RenderManager::getInstance().getViewSize().width;
        int h = MyGUI::RenderManager::getInstance().getViewSize().height;

        mTextColours.header = getTextColour("header");
        mTextColours.normal = getTextColour("normal");
        mTextColours.notify = getTextColour("notify");

        mTextColours.link = getTextColour("link");
        mTextColours.linkOver = getTextColour("link_over");
        mTextColours.linkPressed = getTextColour("link_pressed");

        mTextColours.answer = getTextColour("answer");
        mTextColours.answerOver = getTextColour("answer_over");
        mTextColours.answerPressed = getTextColour("answer_pressed");

        mTextColours.journalLink = getTextColour("journal_link");
        mTextColours.journalLinkOver = getTextColour("journal_link_over");
        mTextColours.journalLinkPressed = getTextColour("journal_link_pressed");

        mTextColours.journalTopic = getTextColour("journal_topic");
        mTextColours.journalTopicOver = getTextColour("journal_topic_over");
        mTextColours.journalTopicPressed = getTextColour("journal_topic_pressed");


        mDragAndDrop = new DragAndDrop();

        Recharge* recharge = new Recharge();
        mGuiModeStates[GM_Recharge] = GuiModeState(recharge);
        mWindows.push_back(recharge);

        MainMenu* menu = new MainMenu(w, h, mResourceSystem->getVFS(), mVersionDescription);
        mGuiModeStates[GM_MainMenu] = GuiModeState(menu);
        mWindows.push_back(menu);

        mLocalMapRender = new MWRender::LocalMap(mViewer->getSceneData()->asGroup());
        mMap = new MapWindow(mCustomMarkers, mDragAndDrop, mLocalMapRender, mWorkQueue);
        mWindows.push_back(mMap);
        mMap->renderGlobalMap();
        trackWindow(mMap, "map");

        mStatsWindow = new StatsWindow(mDragAndDrop);
        mWindows.push_back(mStatsWindow);
        trackWindow(mStatsWindow, "stats");

        mInventoryWindow = new InventoryWindow(mDragAndDrop, mViewer->getSceneData()->asGroup(), mResourceSystem);
        mWindows.push_back(mInventoryWindow);

        mSpellWindow = new SpellWindow(mDragAndDrop);
        mWindows.push_back(mSpellWindow);
        trackWindow(mSpellWindow, "spells");

        mGuiModeStates[GM_Inventory] = GuiModeState({mMap, mInventoryWindow, mSpellWindow, mStatsWindow});
        mGuiModeStates[GM_None] = GuiModeState({mMap, mInventoryWindow, mSpellWindow, mStatsWindow});

        mTradeWindow = new TradeWindow();
        mWindows.push_back(mTradeWindow);
        trackWindow(mTradeWindow, "barter");
        mGuiModeStates[GM_Barter] = GuiModeState({mInventoryWindow, mTradeWindow});

        mConsole = new Console(w,h, mConsoleOnlyScripts);
        mWindows.push_back(mConsole);
        trackWindow(mConsole, "console");
        mGuiModeStates[GM_Console] = GuiModeState(mConsole);

        bool questList = mResourceSystem->getVFS()->exists("textures/tx_menubook_options_over.dds");
        JournalWindow* journal = JournalWindow::create(JournalViewModel::create (), questList, mEncoding);
        mWindows.push_back(journal);
        mGuiModeStates[GM_Journal] = GuiModeState(journal);
        mGuiModeStates[GM_Journal].mCloseSound = "book close";
        mGuiModeStates[GM_Journal].mOpenSound = "book open";

        mMessageBoxManager = new MessageBoxManager(mStore->get<ESM::GameSetting>().find("fMessageTimePerChar")->getFloat());

        SpellBuyingWindow* spellBuyingWindow = new SpellBuyingWindow();
        mWindows.push_back(spellBuyingWindow);
        mGuiModeStates[GM_SpellBuying] = GuiModeState(spellBuyingWindow);

        TravelWindow* travelWindow = new TravelWindow();
        mWindows.push_back(travelWindow);
        mGuiModeStates[GM_Travel] = GuiModeState(travelWindow);

        mDialogueWindow = new DialogueWindow();
        mWindows.push_back(mDialogueWindow);
        trackWindow(mDialogueWindow, "dialogue");
        mGuiModeStates[GM_Dialogue] = GuiModeState(mDialogueWindow);
        mTradeWindow->eventTradeDone += MyGUI::newDelegate(mDialogueWindow, &DialogueWindow::onTradeComplete);

        ContainerWindow* containerWindow = new ContainerWindow(mDragAndDrop);
        mWindows.push_back(containerWindow);
        trackWindow(containerWindow, "container");
        mGuiModeStates[GM_Container] = GuiModeState({containerWindow, mInventoryWindow});

        mHud = new HUD(mCustomMarkers, mDragAndDrop, mLocalMapRender);
        mWindows.push_back(mHud);

        mToolTips = new ToolTips();

        mScrollWindow = new ScrollWindow();
        mWindows.push_back(mScrollWindow);
        mGuiModeStates[GM_Scroll] = GuiModeState(mScrollWindow);
        mGuiModeStates[GM_Scroll].mOpenSound = "scroll";
        mGuiModeStates[GM_Scroll].mCloseSound = "scroll";

        mBookWindow = new BookWindow();
        mWindows.push_back(mBookWindow);
        mGuiModeStates[GM_Book] = GuiModeState(mBookWindow);
        mGuiModeStates[GM_Book].mOpenSound = "book open";
        mGuiModeStates[GM_Book].mCloseSound = "book close";

        mCountDialog = new CountDialog();
        mWindows.push_back(mCountDialog);

        mSettingsWindow = new SettingsWindow();
        mWindows.push_back(mSettingsWindow);
        mGuiModeStates[GM_Settings] = GuiModeState(mSettingsWindow);

        mConfirmationDialog = new ConfirmationDialog();
        mWindows.push_back(mConfirmationDialog);

        AlchemyWindow* alchemyWindow = new AlchemyWindow();
        mWindows.push_back(alchemyWindow);
        trackWindow(alchemyWindow, "alchemy");
        mGuiModeStates[GM_Alchemy] = GuiModeState(alchemyWindow);

        mQuickKeysMenu = new QuickKeysMenu();
        mWindows.push_back(mQuickKeysMenu);
        mGuiModeStates[GM_QuickKeysMenu] = GuiModeState(mQuickKeysMenu);

        LevelupDialog* levelupDialog = new LevelupDialog();
        mWindows.push_back(levelupDialog);
        mGuiModeStates[GM_Levelup] = GuiModeState(levelupDialog);

        mWaitDialog = new WaitDialog();
        mWindows.push_back(mWaitDialog);
        mGuiModeStates[GM_Rest] = GuiModeState({mWaitDialog->getProgressBar(), mWaitDialog});

        SpellCreationDialog* spellCreationDialog = new SpellCreationDialog();
        mWindows.push_back(spellCreationDialog);
        mGuiModeStates[GM_SpellCreation] = GuiModeState(spellCreationDialog);

        EnchantingDialog* enchantingDialog = new EnchantingDialog();
        mWindows.push_back(enchantingDialog);
        mGuiModeStates[GM_Enchanting] = GuiModeState(enchantingDialog);

        TrainingWindow* trainingWindow = new TrainingWindow();
        mWindows.push_back(trainingWindow);
        mGuiModeStates[GM_Training] = GuiModeState({trainingWindow->getProgressBar(), trainingWindow});

        MerchantRepair* merchantRepair = new MerchantRepair();
        mWindows.push_back(merchantRepair);
        mGuiModeStates[GM_MerchantRepair] = GuiModeState(merchantRepair);

        Repair* repair = new Repair();
        mWindows.push_back(repair);
        mGuiModeStates[GM_Repair] = GuiModeState(repair);

        mSoulgemDialog = new SoulgemDialog(mMessageBoxManager);

        CompanionWindow* companionWindow = new CompanionWindow(mDragAndDrop, mMessageBoxManager);
        mWindows.push_back(companionWindow);
        trackWindow(companionWindow, "companion");
        mGuiModeStates[GM_Companion] = GuiModeState({mInventoryWindow, companionWindow});

        mJailScreen = new JailScreen();
        mWindows.push_back(mJailScreen);
        mGuiModeStates[GM_Jail] = GuiModeState(mJailScreen);

        std::string werewolfFaderTex = "textures\\werewolfoverlay.dds";
        if (mResourceSystem->getVFS()->exists(werewolfFaderTex))
        {
            mWerewolfFader = new ScreenFader(werewolfFaderTex);
            mWindows.push_back(mWerewolfFader);
        }
        mBlindnessFader = new ScreenFader("black");
        mWindows.push_back(mBlindnessFader);

        // fall back to player_hit_01.dds if bm_player_hit_01.dds is not available
        std::string hitFaderTexture = "textures\\bm_player_hit_01.dds";
        const std::string hitFaderLayout = "openmw_screen_fader_hit.layout";
        MyGUI::FloatCoord hitFaderCoord (0,0,1,1);
        if(!mResourceSystem->getVFS()->exists(hitFaderTexture))
        {
            hitFaderTexture = "textures\\player_hit_01.dds";
            hitFaderCoord = MyGUI::FloatCoord(0.2, 0.25, 0.6, 0.5);
        }
        mHitFader = new ScreenFader(hitFaderTexture, hitFaderLayout, hitFaderCoord);
        mWindows.push_back(mHitFader);

        mScreenFader = new ScreenFader("black");
        mWindows.push_back(mScreenFader);

        mDebugWindow = new DebugWindow();
        mWindows.push_back(mDebugWindow);

        mInputBlocker = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("",0,0,w,h,MyGUI::Align::Stretch,"InputBlocker");

        mHud->setVisible(true);

        mCharGen = new CharacterCreation(mViewer->getSceneData()->asGroup(), mResourceSystem);

        // Setup player stats
        for (int i = 0; i < ESM::Attribute::Length; ++i)
        {
            mPlayerAttributes.insert(std::make_pair(ESM::Attribute::sAttributeIds[i], MWMechanics::AttributeValue()));
        }

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            mPlayerSkillValues.insert(std::make_pair(ESM::Skill::sSkillIds[i], MWMechanics::SkillValue()));
        }

        updatePinnedWindows();

        // Set up visibility
        updateVisible();
    }

    void WindowManager::setNewGame(bool newgame)
    {
        if (newgame)
        {
            disallowAll();
            delete mCharGen;
            mCharGen = new CharacterCreation(mViewer->getSceneData()->asGroup(), mResourceSystem);
        }
        else
            allow(GW_ALL);
    }

    WindowManager::~WindowManager()
    {
        mKeyboardNavigation.reset();

        MyGUI::LanguageManager::getInstance().eventRequestTag.clear();
        MyGUI::PointerManager::getInstance().eventChangeMousePointer.clear();
        MyGUI::InputManager::getInstance().eventChangeKeyFocus.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

        for (WindowBase* window : mWindows)
            delete window;
        mWindows.clear();

        delete mMessageBoxManager;
        delete mLocalMapRender;
        delete mCharGen;
        delete mDragAndDrop;
        delete mSoulgemDialog;
        delete mCursorManager;
        delete mToolTips;

        cleanupGarbage();

        mFontLoader.reset();

        mGui->shutdown();
        delete mGui;

        mGuiPlatform->shutdown();
        delete mGuiPlatform;
    }

    void WindowManager::setStore(const MWWorld::ESMStore &store)
    {
        mStore = &store;
    }

    void WindowManager::cleanupGarbage()
    {
        // Delete any dialogs which are no longer in use
        if (!mGarbageDialogs.empty())
        {
            for (std::vector<Layout*>::iterator it = mGarbageDialogs.begin(); it != mGarbageDialogs.end(); ++it)
            {
                delete *it;
            }
            mGarbageDialogs.clear();
        }
    }

    void WindowManager::updateVisible()
    {
        if (!mMap)
            return; // UI not created yet

        bool loading = (getMode() == GM_Loading || getMode() == GM_LoadingWallpaper);

        mHud->setVisible(mHudEnabled && !loading);
        mToolTips->setVisible(mHudEnabled && !loading);

        bool gameMode = !isGuiMode();

        MWBase::Environment::get().getInputManager()->changeInputMode(!gameMode);

        mInputBlocker->setVisible (gameMode);

        if (loading)
            setCursorVisible(mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox());
        else
            setCursorVisible(!gameMode);

        if (gameMode)
            setKeyFocusWidget (NULL);

        // Icons of forced hidden windows are displayed
        setMinimapVisibility((mAllowed & GW_Map) && (!mMap->pinned() || (mForceHidden & GW_Map)));
        setWeaponVisibility((mAllowed & GW_Inventory) && (!mInventoryWindow->pinned() || (mForceHidden & GW_Inventory)));
        setSpellVisibility((mAllowed & GW_Magic) && (!mSpellWindow->pinned() || (mForceHidden & GW_Magic)));
        setHMSVisibility((mAllowed & GW_Stats) && (!mStatsWindow->pinned() || (mForceHidden & GW_Stats)));

        mInventoryWindow->setGuiMode(getMode());

        // If in game mode (or interactive messagebox), show the pinned windows
        if (mGuiModes.empty())
        {
            mMap->setVisible(mMap->pinned() && !(mForceHidden & GW_Map) && (mAllowed & GW_Map));
            mStatsWindow->setVisible(mStatsWindow->pinned() && !(mForceHidden & GW_Stats) && (mAllowed & GW_Stats));
            mInventoryWindow->setVisible(mInventoryWindow->pinned() && !(mForceHidden & GW_Inventory) && (mAllowed & GW_Inventory));
            mSpellWindow->setVisible(mSpellWindow->pinned() && !(mForceHidden & GW_Magic) && (mAllowed & GW_Magic));
            return;
        }
        else if (getMode() != GM_Inventory)
        {
            mMap->setVisible(false);
            mStatsWindow->setVisible(false);
            mSpellWindow->setVisible(false);
            mInventoryWindow->setVisible(getMode() == GM_Container || getMode() == GM_Barter || getMode() == GM_Companion);
        }

        GuiMode mode = mGuiModes.back();

        mInventoryWindow->setTrading(mode == GM_Barter);

        if (getMode() == GM_Inventory)
        {
            // For the inventory mode, compute the effective set of windows to show.
            // This is controlled both by what windows the
            // user has opened/closed (the 'shown' variable) and by what
            // windows we are allowed to show (the 'allowed' var.)
            int eff = mShown & mAllowed & ~mForceHidden;
            mMap->setVisible(eff & GW_Map);
            mInventoryWindow->setVisible(eff & GW_Inventory);
            mSpellWindow->setVisible(eff & GW_Magic);
            mStatsWindow->setVisible(eff & GW_Stats);
        }

        switch (mode)
        {
        // FIXME: refactor chargen windows to use modes properly (or not use them at all)
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
        default:
            break;
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

    void WindowManager::removeDialog(Layout*dialog)
    {
        if (!dialog)
            return;
        dialog->setVisible(false);
        mGarbageDialogs.push_back(dialog);
    }

    void WindowManager::exitCurrentGuiMode()
    {
        if (mDragAndDrop && mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->finish();
            return;
        }

        GuiModeState& state = mGuiModeStates[mGuiModes.back()];
        for (WindowBase* window : state.mWindows)
        {
            if (!window->exit())
            {
                // unable to exit window, but give access to main menu
                if (!MyGUI::InputManager::getInstance().isModalAny() && getMode() != GM_MainMenu)
                    pushGuiMode (GM_MainMenu);
                return;
            }
        }

        popGuiMode();
    }

    void WindowManager::interactiveMessageBox(const std::string &message, const std::vector<std::string> &buttons, bool block)
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons);
        updateVisible();

        if (block)
        {
            osg::Timer frameTimer;
            while (mMessageBoxManager->readPressedButton(false) == -1
                   && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
            {
                double dt = frameTimer.time_s();
                frameTimer.setStartTick();

                mKeyboardNavigation->onFrame();
                mMessageBoxManager->onFrame(dt);
                MWBase::Environment::get().getInputManager()->update(dt, true, false);

                if (!MWBase::Environment::get().getInputManager()->isWindowVisible())
                    OpenThreads::Thread::microSleep(5000);
                else
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    mViewer->renderingTraversals();
                }
                // at the time this function is called we are in the middle of a frame,
                // so out of order calls are necessary to get a correct frameNumber for the next frame.
                // refer to the advance() and frame() order in Engine::go()
                mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

                MWBase::Environment::get().limitFrameRate(frameTimer.time_s());
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
        const ESM::GameSetting *setting = mStore->get<ESM::GameSetting>().search(id);

        if (setting && setting->mValue.getType()==ESM::VT_String)
            return setting->mValue.getString();

        return default_;
    }

    void WindowManager::updateMap()
    {
        if (!mLocalMapRender)
            return;

        MWWorld::ConstPtr player = MWMechanics::getPlayer();

        osg::Vec3f playerPosition = player.getRefData().getPosition().asVec3();
        osg::Quat playerOrientation (-player.getRefData().getPosition().rot[2], osg::Vec3(0,0,1));

        osg::Vec3f playerdirection;
        int x,y;
        float u,v;
        mLocalMapRender->updatePlayer(playerPosition, playerOrientation, u, v, x, y, playerdirection);

        if (!player.getCell()->isExterior())
        {
            setActiveMap(x, y, true);
        }
        // else: need to know the current grid center, call setActiveMap from changeCell

        mMap->setPlayerDir(playerdirection.x(), playerdirection.y());
        mMap->setPlayerPos(x, y, u, v);
        mHud->setPlayerDir(playerdirection.x(), playerdirection.y());
        mHud->setPlayerPos(x, y, u, v);
    }

    void WindowManager::onFrame (float frameDuration)
    {
        if (!mGuiModes.empty())
        {
            GuiModeState& state = mGuiModeStates[mGuiModes.back()];
            for (WindowBase* window : state.mWindows)
                window->onFrame(frameDuration);
        }
        else
        {
            // update pinned windows if visible
            for (WindowBase* window : mGuiModeStates[GM_Inventory].mWindows)
                if (window->isVisible())
                    window->onFrame(frameDuration);
        }

        // Make sure message boxes are always in front
        // This is an awful workaround for a series of awfully interwoven issues that couldn't be worked around
        // in a better way because of an impressive number of even more awfully interwoven issues.
        if (mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox() && mCurrentModals.back() != mMessageBoxManager->getInteractiveMessageBox())
        {
            std::vector<WindowModal*>::iterator found = std::find(mCurrentModals.begin(), mCurrentModals.end(), mMessageBoxManager->getInteractiveMessageBox());
            if (found != mCurrentModals.end())
            {
                WindowModal* msgbox = *found;
                std::swap(*found, mCurrentModals.back());
                MyGUI::InputManager::getInstance().addWidgetModal(msgbox->mMainWidget);
                mKeyboardNavigation->setModalWindow(msgbox->mMainWidget);
                mKeyboardNavigation->setDefaultFocus(msgbox->mMainWidget, msgbox->getDefaultKeyFocus());
            }
        }

        if (!mCurrentModals.empty())
            mCurrentModals.back()->onFrame(frameDuration);

        mKeyboardNavigation->onFrame();

        mMessageBoxManager->onFrame(frameDuration);

        mToolTips->onFrame(frameDuration);

        if (mLocalMapRender)
            mLocalMapRender->cleanupCameras();

        if (MWBase::Environment::get().getStateManager()->getState()==
            MWBase::StateManager::State_NoGame)
            return;

        mDragAndDrop->onFrame();

        updateMap();

        mHud->onFrame(frameDuration);

        mDebugWindow->onFrame(frameDuration);

        if (mCharGen)
            mCharGen->onFrame(frameDuration);

        updateActivatedQuickKey ();

        cleanupGarbage();
    }

    void WindowManager::changeCell(const MWWorld::CellStore* cell)
    {
        mMap->requestMapRender(cell);

        std::string name = MWBase::Environment::get().getWorld()->getCellName (cell);

        mMap->setCellName( name );
        mHud->setCellName( name );

        if (cell->getCell()->isExterior())
        {
            if (!cell->getCell()->mName.empty())
                mMap->addVisitedLocation (name, cell->getCell()->getGridX (), cell->getCell()->getGridY ());

            mMap->cellExplored (cell->getCell()->getGridX(), cell->getCell()->getGridY());

            setActiveMap(cell->getCell()->getGridX(), cell->getCell()->getGridY(), false);
        }
        else
        {
            mMap->setCellPrefix (cell->getCell()->mName );
            mHud->setCellPrefix (cell->getCell()->mName );

            osg::Vec3f worldPos;
            if (!MWBase::Environment::get().getWorld()->findInteriorPositionInWorldSpace(cell, worldPos))
                worldPos = MWBase::Environment::get().getWorld()->getPlayer().getLastKnownExteriorPosition();
            else
                MWBase::Environment::get().getWorld()->getPlayer().setLastKnownExteriorPosition(worldPos);
            mMap->setGlobalMapPlayerPosition(worldPos.x(), worldPos.y());

            setActiveMap(0, 0, true);
        }
    }

    void WindowManager::setActiveMap(int x, int y, bool interior)
    {
        mMap->setActiveCell(x,y, interior);
        mHud->setActiveCell(x,y, interior);
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

        if(mHud && (mShowOwned == 2 || mShowOwned == 3))
        {
            bool owned = mToolTips->checkOwned();
            mHud->setCrosshairOwned(owned);
        }
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
        if (visible == mCursorVisible)
            return;
        mCursorVisible = visible;
        if (!visible)
            mCursorActive = false;
    }

    void WindowManager::setCursorActive(bool active)
    {
        mCursorActive = active;
    }

    void WindowManager::onRetrieveTag(const MyGUI::UString& _tag, MyGUI::UString& _result)
    {
        std::string tag(_tag);
        
        std::string MyGuiPrefix = "setting=";
        size_t MyGuiPrefixLength = MyGuiPrefix.length();

        std::string tokenToFind = "sCell=";
        size_t tokenLength = tokenToFind.length();
        
        if(tag.compare(0, MyGuiPrefixLength, MyGuiPrefix) == 0)
        {
            tag = tag.substr(MyGuiPrefixLength, tag.length());
            std::string settingSection = tag.substr(0, tag.find(","));
            std::string settingTag = tag.substr(tag.find(",")+1, tag.length());
            
            _result = Settings::Manager::getString(settingTag, settingSection);            
        }
        else if (tag.compare(0, tokenLength, tokenToFind) == 0)
        {
            _result = mTranslationDataStorage.translateCellName(tag.substr(tokenLength));
        }
        else if (Gui::replaceTag(tag, _result, mFallbackMap))
        {
            return;
        }
        else
        {
            if (!mStore)
            {
                std::cerr << "Error: WindowManager::onRetrieveTag: no Store set up yet, can not replace '" << tag << "'" << std::endl;
                return;
            }
            const ESM::GameSetting *setting = mStore->get<ESM::GameSetting>().find(tag);

            if (setting && setting->mValue.getType()==ESM::VT_String)
                _result = setting->mValue.getString();
            else
                _result = tag;
        }
    }

    void WindowManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        mToolTips->setDelay(Settings::Manager::getFloat("tooltip delay", "GUI"));

        for (Settings::CategorySettingVector::const_iterator it = changed.begin();
            it != changed.end(); ++it)
        {
            if (it->first == "HUD" && it->second == "crosshair")
                mCrosshairEnabled = Settings::Manager::getBool ("crosshair", "HUD");
            else if (it->first == "GUI" && it->second == "subtitles")
                mSubtitlesEnabled = Settings::Manager::getBool ("subtitles", "GUI");
            else if (it->first == "GUI" && it->second == "menu transparency")
                setMenuTransparency(Settings::Manager::getFloat("menu transparency", "GUI"));
        }
    }

    void WindowManager::windowResized(int x, int y)
    {
        mGuiPlatform->getRenderManagerPtr()->setViewSize(x, y);

        // scaled size
        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        x = viewSize.width;
        y = viewSize.height;

        sizeVideo(x, y);

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

        for (WindowBase* window : mWindows)
            window->onResChange(x, y);

        // TODO: check if any windows are now off-screen and move them back if so
    }

    void WindowManager::onCursorChange(const std::string &name)
    {
        mCursorManager->cursorChanged(name);
    }

    void WindowManager::pushGuiMode(GuiMode mode)
    {
        pushGuiMode(mode, MWWorld::Ptr());
    }

    void WindowManager::pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg)
    {
        if (mode==GM_Inventory && mAllowed==GW_None)
            return;

        if (mGuiModes.empty() || mGuiModes.back() != mode)
        {
            // If this mode already exists somewhere in the stack, just bring it to the front.
            if (std::find(mGuiModes.begin(), mGuiModes.end(), mode) != mGuiModes.end())
            {
                mGuiModes.erase(std::find(mGuiModes.begin(), mGuiModes.end(), mode));
            }

            if (!mGuiModes.empty())
            {
                mKeyboardNavigation->saveFocus(mGuiModes.back());
                mGuiModeStates[mGuiModes.back()].update(false);
            }
            mGuiModes.push_back(mode);

            mGuiModeStates[mode].update(true);
            playSound(mGuiModeStates[mode].mOpenSound);
        }
        for (WindowBase* window : mGuiModeStates[mode].mWindows)
            window->setPtr(arg);

        mKeyboardNavigation->restoreFocus(mode);

        updateVisible();
    }

    void WindowManager::popGuiMode(bool noSound)
    {
        if (mDragAndDrop && mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->finish();
        }

        if (!mGuiModes.empty())
        {
            const GuiMode mode = mGuiModes.back();
            mKeyboardNavigation->saveFocus(mode);
            mGuiModes.pop_back();
            mGuiModeStates[mode].update(false);
            if (!noSound)
                playSound(mGuiModeStates[mode].mCloseSound);
        }

        if (!mGuiModes.empty())
        {
            const GuiMode mode = mGuiModes.back();
            mGuiModeStates[mode].update(true);
            mKeyboardNavigation->restoreFocus(mode);
        }

        updateVisible();
    }

    void WindowManager::removeGuiMode(GuiMode mode, bool noSound)
    {
        if (!mGuiModes.empty() && mGuiModes.back() == mode)
        {
            popGuiMode(noSound);
            return;
        }

        std::vector<GuiMode>::iterator it = mGuiModes.begin();
        while (it != mGuiModes.end())
        {
            if (*it == mode)
                it = mGuiModes.erase(it);
            else
                ++it;
        }

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
        mSelectedEnchantItem = MWWorld::Ptr();
        mHud->setSelectedSpell(spellId, successChancePercent);

        const ESM::Spell* spell = mStore->get<ESM::Spell>().find(spellId);

        mSpellWindow->setTitle(spell->mName);
    }

    void WindowManager::setSelectedEnchantItem(const MWWorld::Ptr& item)
    {
        mSelectedEnchantItem = item;
        mSelectedSpell = "";
        const ESM::Enchantment* ench = mStore->get<ESM::Enchantment>()
                .find(item.getClass().getEnchantment(item));

        int chargePercent = (item.getCellRef().getEnchantmentCharge() == -1) ? 100
                : static_cast<int>(item.getCellRef().getEnchantmentCharge() / static_cast<float>(ench->mData.mCharge) * 100);
        mHud->setSelectedEnchantItem(item, chargePercent);
        mSpellWindow->setTitle(item.getClass().getName(item));
    }

    const MWWorld::Ptr &WindowManager::getSelectedEnchantItem() const
    {
        return mSelectedEnchantItem;
    }

    void WindowManager::setSelectedWeapon(const MWWorld::Ptr& item)
    {
        mSelectedWeapon = item;
        int durabilityPercent =
             static_cast<int>(item.getClass().getItemHealth(item) / static_cast<float>(item.getClass().getItemMaxHealth(item)) * 100);
        mHud->setSelectedWeapon(item, durabilityPercent);
        mInventoryWindow->setTitle(item.getClass().getName(item));
    }

    const MWWorld::Ptr &WindowManager::getSelectedWeapon() const
    {
        return mSelectedWeapon;
    }

    void WindowManager::unsetSelectedSpell()
    {
        mSelectedSpell = "";
        mSelectedEnchantItem = MWWorld::Ptr();
        mHud->unsetSelectedSpell();

        MWWorld::Player* player = &MWBase::Environment::get().getWorld()->getPlayer();
        if (player->getDrawState() == MWMechanics::DrawState_Spell)
            player->setDrawState(MWMechanics::DrawState_Nothing);

        mSpellWindow->setTitle("#{sNone}");
    }

    void WindowManager::unsetSelectedWeapon()
    {
        mSelectedWeapon = MWWorld::Ptr();
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

    MWGui::InventoryWindow* WindowManager::getInventoryWindow() { return mInventoryWindow; }
    MWGui::CountDialog* WindowManager::getCountDialog() { return mCountDialog; }
    MWGui::ConfirmationDialog* WindowManager::getConfirmationDialog() { return mConfirmationDialog; }
    MWGui::TradeWindow* WindowManager::getTradeWindow() { return mTradeWindow; }

    void WindowManager::useItem(const MWWorld::Ptr &item)
    {
        if (mInventoryWindow)
            mInventoryWindow->useItem(item);
    }

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
        mRestAllowed = false;

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

    void WindowManager::updateActivatedQuickKey ()
    {
        mQuickKeysMenu->updateActivatedQuickKey();
    }

    void WindowManager::activateQuickKey (int index)
    {
        mQuickKeysMenu->activateQuickKey(index);
    }

    bool WindowManager::getSubtitlesEnabled ()
    {
        return mSubtitlesEnabled;
    }

    bool WindowManager::toggleHud()
    {
        mHudEnabled = !mHudEnabled;
        updateVisible();
        return mHudEnabled;
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

    const Translation::Storage& WindowManager::getTranslationDataStorage() const
    {
        return mTranslationDataStorage;
    }

    void WindowManager::changePointer(const std::string &name)
    {
        MyGUI::PointerManager::getInstance().setPointer(name);
        onCursorChange(name);
    }

    void WindowManager::showSoulgemDialog(MWWorld::Ptr item)
    {
        mSoulgemDialog->show(item);
        updateVisible();
    }

    void WindowManager::updatePlayer()
    {
        mInventoryWindow->updatePlayer();

        const MWWorld::Ptr player = MWMechanics::getPlayer();
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

    bool WindowManager::getCursorVisible()
    {
        return mCursorVisible && mCursorActive;
    }

    void WindowManager::trackWindow(Layout *layout, const std::string &name)
    {
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::IntPoint pos(static_cast<int>(Settings::Manager::getFloat(name + " x", "Windows") * viewSize.width),
                            static_cast<int>(Settings::Manager::getFloat(name + " y", "Windows") * viewSize.height));
        MyGUI::IntSize size (static_cast<int>(Settings::Manager::getFloat(name + " w", "Windows") * viewSize.width),
                             static_cast<int>(Settings::Manager::getFloat(name + " h", "Windows") * viewSize.height));
        layout->mMainWidget->setPosition(pos);
        layout->mMainWidget->setSize(size);

        MyGUI::Window* window = layout->mMainWidget->castType<MyGUI::Window>();
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
        for (WindowBase* window : mWindows)
            window->clear();

        if (mLocalMapRender)
            mLocalMapRender->clear();

        mMessageBoxManager->clear();

        mToolTips->setFocusObject(MWWorld::Ptr());

        mSelectedSpell.clear();
        mCustomMarkers.clear();

        mForceHidden = GW_None;
        mRestAllowed = true;

        while (!mGuiModes.empty())
            popGuiMode();

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

        for (CustomMarkerCollection::ContainerType::const_iterator it = mCustomMarkers.begin(); it != mCustomMarkers.end(); ++it)
        {
            writer.startRecord(ESM::REC_MARK);
            it->second.save(writer);
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
            std::string spell = reader.getHString();
            if (mStore->get<ESM::Spell>().search(spell))
                mSelectedSpell = spell;
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
                && (!isGuiMode() || (mGuiModes.size() == 1 && (getMode() == GM_MainMenu || getMode() == GM_Rest)));
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
        int oldUpdateMask = mViewer->getUpdateVisitor()->getTraversalMask();
        int oldCullMask = mViewer->getCamera()->getCullMask();
        mViewer->getUpdateVisitor()->setTraversalMask(MWRender::Mask_GUI);
        mViewer->getCamera()->setCullMask(MWRender::Mask_GUI);

        MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
        sizeVideo(screenSize.width, screenSize.height);

        MyGUI::Widget* oldKeyFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        setKeyFocusWidget(mVideoWidget);

        mVideoBackground->setVisible(true);

        bool cursorWasVisible = mCursorVisible;
        setCursorVisible(false);

        if (mVideoWidget->hasAudioStream())
            MWBase::Environment::get().getSoundManager()->pauseSounds(
                ~MWSound::Type::Movie & MWSound::Type::Mask
            );
        osg::Timer frameTimer;
        while (mVideoWidget->update() && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
        {
            double dt = frameTimer.time_s();
            frameTimer.setStartTick();

            MWBase::Environment::get().getInputManager()->update(dt, true, false);

            if (!MWBase::Environment::get().getInputManager()->isWindowVisible())
                OpenThreads::Thread::microSleep(5000);
            else
            {
                mViewer->eventTraversal();
                mViewer->updateTraversal();
                mViewer->renderingTraversals();
            }
            // at the time this function is called we are in the middle of a frame,
            // so out of order calls are necessary to get a correct frameNumber for the next frame.
            // refer to the advance() and frame() order in Engine::go()
            mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

            MWBase::Environment::get().limitFrameRate(frameTimer.time_s());
        }
        mVideoWidget->stop();

        MWBase::Environment::get().getSoundManager()->resumeSounds();

        setKeyFocusWidget(oldKeyFocus);

        setCursorVisible(cursorWasVisible);

        // Restore normal rendering
        mViewer->getUpdateVisitor()->setTraversalMask(oldUpdateMask);
        mViewer->getCamera()->setCullMask(oldCullMask);

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
        {
            WindowModal* window = mCurrentModals.back();
            if (!window->exit())
                return;
            window->setVisible(false);
        }
    }

    void WindowManager::addCurrentModal(WindowModal *input)
    {
        if (mCurrentModals.empty())
            mKeyboardNavigation->saveFocus(getMode());

        mCurrentModals.push_back(input);
        mKeyboardNavigation->restoreFocus(-1);

        mKeyboardNavigation->setModalWindow(input->mMainWidget);
        mKeyboardNavigation->setDefaultFocus(input->mMainWidget, input->getDefaultKeyFocus());
    }

    void WindowManager::removeCurrentModal(WindowModal* input)
    {
        if(!mCurrentModals.empty())
        {
            if(input == mCurrentModals.back())
            {
                mCurrentModals.pop_back();
                mKeyboardNavigation->saveFocus(-1);
            }
            else
            {
                auto found = std::find(mCurrentModals.begin(), mCurrentModals.end(), input);
                if (found != mCurrentModals.end())
                    mCurrentModals.erase(found);
                else
                    std::cerr << " warning: can't find modal window " << input << std::endl;
            }
        }
        if (mCurrentModals.empty())
        {
            mKeyboardNavigation->setModalWindow(NULL);
            mKeyboardNavigation->restoreFocus(getMode());
        }
        else
            mKeyboardNavigation->setModalWindow(mCurrentModals.back()->mMainWidget);
    }

    void WindowManager::onVideoKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
    {
        if (_key == MyGUI::KeyCode::Escape)
            mVideoWidget->stop();
    }

    void WindowManager::updatePinnedWindows()
    {
        mInventoryWindow->setPinned(Settings::Manager::getBool("inventory pin", "Windows"));

        mMap->setPinned(Settings::Manager::getBool("map pin", "Windows"));

        mSpellWindow->setPinned(Settings::Manager::getBool("spells pin", "Windows"));

        mStatsWindow->setPinned(Settings::Manager::getBool("stats pin", "Windows"));
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

    void WindowManager::fadeScreenIn(const float time, bool clearQueue, float delay)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeOut(time, delay);
    }

    void WindowManager::fadeScreenOut(const float time, bool clearQueue, float delay)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeIn(time, delay);
    }

    void WindowManager::fadeScreenTo(const int percent, const float time, bool clearQueue, float delay)
    {
        if (clearQueue)
            mScreenFader->clearQueue();
        mScreenFader->fadeTo(percent, time, delay);
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

        if (mWerewolfFader)
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
            _data = MyGUI::TextIterator::toTagsString(text);

        SDL_free(text);
    }

    void WindowManager::toggleDebugWindow()
    {
        mDebugWindow->setVisible(!mDebugWindow->isVisible());
    }

    void WindowManager::cycleSpell(bool next)
    {
        if (!isGuiMode())
            mSpellWindow->cycle(next);
    }

    void WindowManager::cycleWeapon(bool next)
    {
        if (!isGuiMode())
            mInventoryWindow->cycle(next);
    }

    void WindowManager::playSound(const std::string& soundId, bool preventOverlapping, float volume, float pitch)
    {
        if (soundId.empty())
            return;

        MWBase::SoundManager *sndmgr = MWBase::Environment::get().getSoundManager();
        if (preventOverlapping && sndmgr->getSoundPlaying(MWWorld::Ptr(), soundId))
            return;

        sndmgr->playSound(soundId, volume, pitch, MWSound::Type::Sfx, MWSound::PlayMode::NoEnv);
    }

    void WindowManager::updateSpellWindow()
    {
        if (mSpellWindow)
            mSpellWindow->updateSpells();
    }

    void WindowManager::setConsoleSelectedObject(const MWWorld::Ptr &object)
    {
        mConsole->setSelectedObject(object);
    }

    std::string WindowManager::correctIconPath(const std::string& path)
    {
        return Misc::ResourceHelpers::correctIconPath(path, mResourceSystem->getVFS());
    }

    std::string WindowManager::correctBookartPath(const std::string& path, int width, int height, bool* exists)
    {
        std::string corrected = Misc::ResourceHelpers::correctBookartPath(path, width, height, mResourceSystem->getVFS());
        if (exists)
            *exists = mResourceSystem->getVFS()->exists(corrected);
        return corrected;
    }

    std::string WindowManager::correctTexturePath(const std::string& path)
    {
        return Misc::ResourceHelpers::correctTexturePath(path, mResourceSystem->getVFS());
    }

    bool WindowManager::textureExists(const std::string &path)
    {
        std::string corrected = Misc::ResourceHelpers::correctTexturePath(path, mResourceSystem->getVFS());
        return mResourceSystem->getVFS()->exists(corrected);
    }

    void WindowManager::createCursors()
    {
        MyGUI::ResourceManager::EnumeratorPtr enumerator = MyGUI::ResourceManager::getInstance().getEnumerator();
        while (enumerator.next())
        {
            MyGUI::IResource* resource = enumerator.current().second;
            ResourceImageSetPointerFix* imgSetPointer = resource->castType<ResourceImageSetPointerFix>(false);
            if (!imgSetPointer)
                continue;
            std::string tex_name = imgSetPointer->getImageSet()->getIndexInfo(0,0).texture;

            osg::ref_ptr<osg::Image> image = mResourceSystem->getImageManager()->getImage(tex_name);

            if(image.valid())
            {
                //everything looks good, send it to the cursor manager
                Uint8 hotspot_x = imgSetPointer->getHotSpot().left;
                Uint8 hotspot_y = imgSetPointer->getHotSpot().top;
                int rotation = imgSetPointer->getRotation();

                mCursorManager->createCursor(imgSetPointer->getResourceName(), rotation, image, hotspot_x, hotspot_y);
            }
        }
    }

    void WindowManager::createTextures()
    {
        {
            MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture("white");
            tex->createManual(8, 8, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8);
            unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
            for (int x=0; x<8; ++x)
                for (int y=0; y<8; ++y)
                {
                    *(data++) = 255;
                    *(data++) = 255;
                    *(data++) = 255;
                }
            tex->unlock();
        }

        {
            MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture("black");
            tex->createManual(8, 8, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8);
            unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
            for (int x=0; x<8; ++x)
                for (int y=0; y<8; ++y)
                {
                    *(data++) = 0;
                    *(data++) = 0;
                    *(data++) = 0;
                }
            tex->unlock();
        }

        {
            MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture("transparent");
            tex->createManual(8, 8, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8A8);
            setMenuTransparency(Settings::Manager::getFloat("menu transparency", "GUI"));
        }
    }

    void WindowManager::setMenuTransparency(float value)
    {
        MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().getTexture("transparent");
        unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
        for (int x=0; x<8; ++x)
            for (int y=0; y<8; ++y)
            {
                *(data++) = 255;
                *(data++) = 255;
                *(data++) = 255;
                *(data++) = static_cast<unsigned char>(value*255);
            }
        tex->unlock();
    }

    void WindowManager::removeCell(MWWorld::CellStore *cell)
    {
        mLocalMapRender->removeCell(cell);
    }

    void WindowManager::writeFog(MWWorld::CellStore *cell)
    {
        mLocalMapRender->saveFogOfWar(cell);
    }

    const MWGui::TextColours& WindowManager::getTextColours()
    {
        return mTextColours;
    }

    bool WindowManager::injectKeyPress(MyGUI::KeyCode key, unsigned int text)
    {
        if (!mKeyboardNavigation->injectKeyPress(key, text))
        {
            MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
            bool widgetActive = MyGUI::InputManager::getInstance().injectKeyPress(key, text);
            if (!widgetActive || !focus)
                return false;
            // FIXME: MyGUI doesn't allow widgets to state if a given key was actually used, so make a guess
            if (focus->getTypeName().find("Button") != std::string::npos)
            {
                switch (key.getValue())
                {
                case MyGUI::KeyCode::ArrowDown:
                case MyGUI::KeyCode::ArrowUp:
                case MyGUI::KeyCode::ArrowLeft:
                case MyGUI::KeyCode::ArrowRight:
                case MyGUI::KeyCode::Return:
                case MyGUI::KeyCode::NumpadEnter:
                case MyGUI::KeyCode::Space:
                    return true;
                default:
                    return false;
                }
            }
            return false;
        }
        else
            return true;
    }

    void WindowManager::GuiModeState::update(bool visible)
    {
        for (unsigned int i=0; i<mWindows.size(); ++i)
            mWindows[i]->setVisible(visible);
    }
}
