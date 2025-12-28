#include "windowmanagerimp.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>

#include <osgViewer/Viewer>

#include <MyGUI_ClipboardManager.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_PointerManager.h>
#include <MyGUI_UString.h>

// For BT_NO_PROFILE
#include <LinearMath/btQuickprof.h>

#include <SDL_clipboard.h>
#include <SDL_keyboard.h>

#include <components/debug/debuglog.hpp>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/fontloader/fontloader.hpp>

#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/workqueue.hpp>

#include <components/translation/translation.hpp>

#include <components/myguiplatform/additivelayer.hpp>
#include <components/myguiplatform/myguiplatform.hpp>
#include <components/myguiplatform/myguirendermanager.hpp>
#include <components/myguiplatform/scalinglayer.hpp>

#include <components/vfs/manager.hpp>

#include <components/widgets/tags.hpp>
#include <components/widgets/widgets.hpp>

#include <components/misc/frameratelimiter.hpp>

#include <components/l10n/manager.hpp>

#include <components/lua_ui/util.hpp>
#include <components/lua_ui/widget.hpp>

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/vismask.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/globals.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwrender/postprocessor.hpp"

#include "alchemywindow.hpp"
#include "backgroundimage.hpp"
#include "bookpage.hpp"
#include "bookwindow.hpp"
#include "companionwindow.hpp"
#include "confirmationdialog.hpp"
#include "console.hpp"
#include "container.hpp"
#include "controllerbuttonsoverlay.hpp"
#include "controllers.hpp"
#include "countdialog.hpp"
#include "cursor.hpp"
#include "debugwindow.hpp"
#include "dialogue.hpp"
#include "enchantingdialog.hpp"
#include "exposedwindow.hpp"
#include "hud.hpp"
#include "inventorytabsoverlay.hpp"
#include "inventorywindow.hpp"
#include "itemchargeview.hpp"
#include "itemtransfer.hpp"
#include "itemview.hpp"
#include "itemwidget.hpp"
#include "jailscreen.hpp"
#include "journalviewmodel.hpp"
#include "journalwindow.hpp"
#include "keyboardnavigation.hpp"
#include "levelupdialog.hpp"
#include "loadingscreen.hpp"
#include "mainmenu.hpp"
#include "merchantrepair.hpp"
#include "postprocessorhud.hpp"
#include "quickkeysmenu.hpp"
#include "recharge.hpp"
#include "repair.hpp"
#include "resourceskin.hpp"
#include "screenfader.hpp"
#include "scrollwindow.hpp"
#include "settingswindow.hpp"
#include "spellbuyingwindow.hpp"
#include "spellview.hpp"
#include "spellwindow.hpp"
#include "statswindow.hpp"
#include "tradewindow.hpp"
#include "trainingwindow.hpp"
#include "travelwindow.hpp"
#include "videowidget.hpp"
#include "waitdialog.hpp"

namespace MWGui
{
    namespace
    {
        Settings::SettingValue<bool>* findHiddenSetting(GuiWindow window)
        {
            switch (window)
            {
                case GW_Inventory:
                    return &Settings::windows().mInventoryHidden;
                case GW_Map:
                    return &Settings::windows().mMapHidden;
                case GW_Magic:
                    return &Settings::windows().mSpellsHidden;
                case GW_Stats:
                    return &Settings::windows().mStatsHidden;
                default:
                    return nullptr;
            }
        }
    }

    WindowManager::WindowManager(SDL_Window* window, osgViewer::Viewer* viewer, osg::Group* guiRoot,
        Resource::ResourceSystem* resourceSystem, SceneUtil::WorkQueue* workQueue, const std::filesystem::path& logpath,
        bool consoleOnlyScripts, Translation::Storage& translationDataStorage, ToUTF8::FromType encoding,
        bool exportFonts, const std::string& versionDescription, bool useShaders, Files::ConfigurationManager& cfgMgr)
        : mOldUpdateMask(0)
        , mOldCullMask(0)
        , mStore(nullptr)
        , mResourceSystem(resourceSystem)
        , mWorkQueue(workQueue)
        , mViewer(viewer)
        , mConsoleOnlyScripts(consoleOnlyScripts)
        , mCurrentModals()
        , mHud(nullptr)
        , mMap(nullptr)
        , mStatsWindow(nullptr)
        , mConsole(nullptr)
        , mDialogueWindow(nullptr)
        , mInventoryWindow(nullptr)
        , mScrollWindow(nullptr)
        , mBookWindow(nullptr)
        , mCountDialog(nullptr)
        , mTradeWindow(nullptr)
        , mSettingsWindow(nullptr)
        , mConfirmationDialog(nullptr)
        , mSpellWindow(nullptr)
        , mQuickKeysMenu(nullptr)
        , mLoadingScreen(nullptr)
        , mWaitDialog(nullptr)
        , mVideoBackground(nullptr)
        , mVideoWidget(nullptr)
        , mWerewolfFader(nullptr)
        , mBlindnessFader(nullptr)
        , mHitFader(nullptr)
        , mScreenFader(nullptr)
        , mDebugWindow(nullptr)
        , mPostProcessorHud(nullptr)
        , mJailScreen(nullptr)
        , mContainerWindow(nullptr)
        , mControllerButtonsOverlay(nullptr)
        , mInventoryTabsOverlay(nullptr)
        , mTranslationDataStorage(translationDataStorage)
        , mInputBlocker(nullptr)
        , mHudEnabled(true)
        , mCursorVisible(true)
        , mCursorActive(true)
        , mPlayerBounty(-1)
        , mGuiModes()
        , mGarbageDialogs()
        , mShown(GW_ALL)
        , mForceHidden(GW_None)
        , mAllowed(GW_ALL)
        , mRestAllowed(true)
        , mEncoding(encoding)
        , mVersionDescription(versionDescription)
        , mWindowVisible(true)
        , mCfgMgr(cfgMgr)
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        int dw, dh;
        SDL_GL_GetDrawableSize(window, &dw, &dh);

        mScalingFactor = Settings::gui().mScalingFactor * (dw / w);
        constexpr VFS::Path::NormalizedView resourcePath("mygui");
        mGuiPlatform = std::make_unique<MyGUIPlatform::Platform>(viewer, guiRoot, resourceSystem->getImageManager(),
            resourceSystem->getVFS(), mScalingFactor, resourcePath, logpath / "MyGUI.log");

        mGui = std::make_unique<MyGUI::Gui>();
        mGui->initialise({});

        createTextures();

        MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &WindowManager::onRetrieveTag);

        // Load fonts
        mFontLoader
            = std::make_unique<Gui::FontLoader>(encoding, resourceSystem->getVFS(), mScalingFactor, exportFonts);

        // Register own widgets with MyGUI
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWEffectList>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Window>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<VideoWidget>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<BackgroundImage>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<MyGUIPlatform::AdditiveLayer>("Layer");
        MyGUI::FactoryManager::getInstance().registerFactory<MyGUIPlatform::ScalingLayer>("Layer");
        BookPage::registerMyGUIComponents();
        PostProcessorHud::registerMyGUIComponents();
        ItemView::registerComponents();
        ItemChargeView::registerComponents();
        ItemWidget::registerComponents();
        SpellView::registerComponents();
        Gui::registerAllWidgets();
        LuaUi::registerAllWidgets();

        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Controllers::ControllerFollowMouse>("Controller");

        MyGUI::FactoryManager::getInstance().registerFactory<ResourceImageSetPointerFix>(
            "Resource", "ResourceImageSetPointer");
        MyGUI::FactoryManager::getInstance().registerFactory<AutoSizedResourceSkin>(
            "Resource", "AutoSizedResourceSkin");
        MyGUI::ResourceManager::getInstance().load("core.xml");

        const bool keyboardNav = Settings::gui().mKeyboardNavigation;
        mKeyboardNavigation = std::make_unique<KeyboardNavigation>();
        mKeyboardNavigation->setEnabled(keyboardNav);
        Gui::ImageButton::setDefaultNeedKeyFocus(keyboardNav);

        auto loadingScreen = std::make_unique<LoadingScreen>(mResourceSystem, mViewer);
        mLoadingScreen = loadingScreen.get();
        mWindows.push_back(std::move(loadingScreen));

        // set up the hardware cursor manager
        mCursorManager = std::make_unique<SDLUtil::SDLCursorManager>();

        MyGUI::PointerManager::getInstance().eventChangeMousePointer
            += MyGUI::newDelegate(this, &WindowManager::onCursorChange);

        MyGUI::InputManager::getInstance().eventChangeKeyFocus
            += MyGUI::newDelegate(this, &WindowManager::onKeyFocusChanged);

        // Create all cursors in advance
        createCursors();
        onCursorChange(MyGUI::PointerManager::getInstance().getDefaultPointer());
        mCursorManager->setEnabled(true);

        // hide mygui's pointer
        MyGUI::PointerManager::getInstance().setVisible(false);

        mVideoBackground = MyGUI::Gui::getInstance().createWidgetReal<MyGUI::ImageBox>(
            "ImageBox", 0, 0, 1, 1, MyGUI::Align::Default, "Video");
        mVideoBackground->setImageTexture("black");
        mVideoBackground->setVisible(false);
        mVideoBackground->setNeedMouseFocus(true);
        mVideoBackground->setNeedKeyFocus(true);

        mVideoWidget = mVideoBackground->createWidgetReal<VideoWidget>("ImageBox", 0, 0, 1, 1, MyGUI::Align::Default);
        mVideoWidget->setNeedMouseFocus(true);
        mVideoWidget->setNeedKeyFocus(true);
        mVideoWidget->setVFS(resourceSystem->getVFS());

        // Removes default MyGUI system clipboard implementation, which supports windows only
        MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

        MyGUI::ClipboardManager::getInstance().eventClipboardChanged
            += MyGUI::newDelegate(this, &WindowManager::onClipboardChanged);
        MyGUI::ClipboardManager::getInstance().eventClipboardRequested
            += MyGUI::newDelegate(this, &WindowManager::onClipboardRequested);

        mVideoWrapper = std::make_unique<SDLUtil::VideoWrapper>(window, viewer);
        mVideoWrapper->setGammaContrast(Settings::video().mGamma, Settings::video().mContrast);

        if (useShaders)
            mGuiPlatform->getRenderManagerPtr()->enableShaders(mResourceSystem->getSceneManager()->getShaderManager());

        mStatsWatcher = std::make_unique<StatsWatcher>();
    }

    void WindowManager::initUI()
    {
        // Get size info from the Gui object
        int w = MyGUI::RenderManager::getInstance().getViewSize().width;
        int h = MyGUI::RenderManager::getInstance().getViewSize().height;

        mTextColours.loadColours();

        mDragAndDrop = std::make_unique<DragAndDrop>();
        mItemTransfer = std::make_unique<ItemTransfer>(*this);

        auto recharge = std::make_unique<Recharge>();
        mGuiModeStates[GM_Recharge] = GuiModeState(recharge.get());
        mWindows.push_back(std::move(recharge));

        auto menu = std::make_unique<MainMenu>(w, h, mResourceSystem->getVFS(), mVersionDescription);
        mGuiModeStates[GM_MainMenu] = GuiModeState(menu.get());
        mWindows.push_back(std::move(menu));

        mLocalMapRender = std::make_unique<MWRender::LocalMap>(mViewer->getSceneData()->asGroup());
        auto map = std::make_unique<MapWindow>(mCustomMarkers, mDragAndDrop.get(), mLocalMapRender.get(), mWorkQueue);
        mMap = map.get();
        mWindows.push_back(std::move(map));
        mMap->renderGlobalMap();
        trackWindow(mMap, makeMapWindowSettingValues());

        auto statsWindow = std::make_unique<StatsWindow>(mDragAndDrop.get());
        mStatsWindow = statsWindow.get();
        mWindows.push_back(std::move(statsWindow));
        trackWindow(mStatsWindow, makeStatsWindowSettingValues());

        auto inventoryWindow = std::make_unique<InventoryWindow>(
            *mDragAndDrop, *mItemTransfer, mViewer->getSceneData()->asGroup(), mResourceSystem);
        mInventoryWindow = inventoryWindow.get();
        mWindows.push_back(std::move(inventoryWindow));

        auto spellWindow = std::make_unique<SpellWindow>(mDragAndDrop.get());
        mSpellWindow = spellWindow.get();
        mWindows.push_back(std::move(spellWindow));
        trackWindow(mSpellWindow, makeSpellsWindowSettingValues());

        mGuiModeStates[GM_Inventory] = GuiModeState({ mMap, mInventoryWindow, mSpellWindow, mStatsWindow });
        mGuiModeStates[GM_None] = GuiModeState({ mMap, mInventoryWindow, mSpellWindow, mStatsWindow });

        auto tradeWindow = std::make_unique<TradeWindow>();
        mTradeWindow = tradeWindow.get();
        mWindows.push_back(std::move(tradeWindow));
        trackWindow(mTradeWindow, makeBarterWindowSettingValues());
        mGuiModeStates[GM_Barter] = GuiModeState({ mInventoryWindow, mTradeWindow });

        auto console = std::make_unique<Console>(w, h, mConsoleOnlyScripts, mCfgMgr);
        mConsole = console.get();
        mWindows.push_back(std::move(console));
        trackWindow(mConsole, makeConsoleWindowSettingValues());

        constexpr VFS::Path::NormalizedView menubookOptionsOverTexture("textures/tx_menubook_options_over.dds");
        const bool questList = mResourceSystem->getVFS()->exists(menubookOptionsOverTexture);
        auto journal = JournalWindow::create(JournalViewModel::create(), questList, mEncoding);
        mGuiModeStates[GM_Journal] = GuiModeState(journal.get());
        mWindows.push_back(std::move(journal));

        mMessageBoxManager = std::make_unique<MessageBoxManager>(
            mStore->get<ESM::GameSetting>().find("fMessageTimePerChar")->mValue.getFloat());

        auto spellBuyingWindow = std::make_unique<SpellBuyingWindow>();
        mGuiModeStates[GM_SpellBuying] = GuiModeState(spellBuyingWindow.get());
        mWindows.push_back(std::move(spellBuyingWindow));

        auto travelWindow = std::make_unique<TravelWindow>();
        mGuiModeStates[GM_Travel] = GuiModeState(travelWindow.get());
        mWindows.push_back(std::move(travelWindow));

        auto dialogueWindow = std::make_unique<DialogueWindow>();
        mDialogueWindow = dialogueWindow.get();
        mWindows.push_back(std::move(dialogueWindow));
        trackWindow(mDialogueWindow, makeDialogueWindowSettingValues());
        mGuiModeStates[GM_Dialogue] = GuiModeState(mDialogueWindow);
        mTradeWindow->eventTradeDone += MyGUI::newDelegate(mDialogueWindow, &DialogueWindow::onTradeComplete);

        auto containerWindow = std::make_unique<ContainerWindow>(*mDragAndDrop, *mItemTransfer);
        mContainerWindow = containerWindow.get();
        mWindows.push_back(std::move(containerWindow));
        trackWindow(mContainerWindow, makeContainerWindowSettingValues());
        mGuiModeStates[GM_Container] = GuiModeState({ mContainerWindow, mInventoryWindow });

        auto hud = std::make_unique<HUD>(mCustomMarkers, mDragAndDrop.get(), mLocalMapRender.get());
        mHud = hud.get();
        mWindows.push_back(std::move(hud));

        mToolTips = std::make_unique<ToolTips>();

        auto scrollWindow = std::make_unique<ScrollWindow>();
        mScrollWindow = scrollWindow.get();
        mWindows.push_back(std::move(scrollWindow));
        mGuiModeStates[GM_Scroll] = GuiModeState(mScrollWindow);

        auto bookWindow = std::make_unique<BookWindow>();
        mBookWindow = bookWindow.get();
        mWindows.push_back(std::move(bookWindow));
        mGuiModeStates[GM_Book] = GuiModeState(mBookWindow);

        auto countDialog = std::make_unique<CountDialog>();
        mCountDialog = countDialog.get();
        mWindows.push_back(std::move(countDialog));

        auto settingsWindow = std::make_unique<SettingsWindow>(mCfgMgr);
        mSettingsWindow = settingsWindow.get();
        mWindows.push_back(std::move(settingsWindow));
        trackWindow(mSettingsWindow, makeSettingsWindowSettingValues());

        auto confirmationDialog = std::make_unique<ConfirmationDialog>();
        mConfirmationDialog = confirmationDialog.get();
        mWindows.push_back(std::move(confirmationDialog));

        auto alchemyWindow = std::make_unique<AlchemyWindow>();
        trackWindow(alchemyWindow.get(), makeAlchemyWindowSettingValues());
        mGuiModeStates[GM_Alchemy] = GuiModeState(alchemyWindow.get());
        mWindows.push_back(std::move(alchemyWindow));

        auto quickKeysMenu = std::make_unique<QuickKeysMenu>();
        mQuickKeysMenu = quickKeysMenu.get();
        mWindows.push_back(std::move(quickKeysMenu));
        mGuiModeStates[GM_QuickKeysMenu] = GuiModeState(mQuickKeysMenu);

        auto levelupDialog = std::make_unique<LevelupDialog>();
        mGuiModeStates[GM_Levelup] = GuiModeState(levelupDialog.get());
        mWindows.push_back(std::move(levelupDialog));

        auto waitDialog = std::make_unique<WaitDialog>();
        mWaitDialog = waitDialog.get();
        mWindows.push_back(std::move(waitDialog));
        mGuiModeStates[GM_Rest] = GuiModeState({ mWaitDialog->getProgressBar(), mWaitDialog });

        auto spellCreationDialog = std::make_unique<SpellCreationDialog>();
        mGuiModeStates[GM_SpellCreation] = GuiModeState(spellCreationDialog.get());
        mWindows.push_back(std::move(spellCreationDialog));

        auto enchantingDialog = std::make_unique<EnchantingDialog>();
        mGuiModeStates[GM_Enchanting] = GuiModeState(enchantingDialog.get());
        mWindows.push_back(std::move(enchantingDialog));

        auto trainingWindow = std::make_unique<TrainingWindow>();
        mGuiModeStates[GM_Training] = GuiModeState({ trainingWindow->getProgressBar(), trainingWindow.get() });
        mWindows.push_back(std::move(trainingWindow));

        auto merchantRepair = std::make_unique<MerchantRepair>();
        mGuiModeStates[GM_MerchantRepair] = GuiModeState(merchantRepair.get());
        mWindows.push_back(std::move(merchantRepair));

        auto repair = std::make_unique<Repair>();
        mGuiModeStates[GM_Repair] = GuiModeState(repair.get());
        mWindows.push_back(std::move(repair));

        mSoulgemDialog = std::make_unique<SoulgemDialog>(mMessageBoxManager.get());

        auto companionWindow
            = std::make_unique<CompanionWindow>(*mDragAndDrop, *mItemTransfer, mMessageBoxManager.get());
        trackWindow(companionWindow.get(), makeCompanionWindowSettingValues());
        mGuiModeStates[GM_Companion] = GuiModeState({ mInventoryWindow, companionWindow.get() });
        mWindows.push_back(std::move(companionWindow));

        auto jailScreen = std::make_unique<JailScreen>();
        mJailScreen = jailScreen.get();
        mWindows.push_back(std::move(jailScreen));
        mGuiModeStates[GM_Jail] = GuiModeState(mJailScreen);

        std::string werewolfFaderTex = "textures\\werewolfoverlay.dds";
        if (mResourceSystem->getVFS()->exists(werewolfFaderTex))
        {
            auto werewolfFader = std::make_unique<ScreenFader>(werewolfFaderTex);
            mWerewolfFader = werewolfFader.get();
            mWindows.push_back(std::move(werewolfFader));
        }
        auto blindnessFader = std::make_unique<ScreenFader>("black");
        mBlindnessFader = blindnessFader.get();
        mWindows.push_back(std::move(blindnessFader));

        // fall back to player_hit_01.dds if bm_player_hit_01.dds is not available
        std::string hitFaderTexture = "textures\\bm_player_hit_01.dds";
        const std::string hitFaderLayout = "openmw_screen_fader_hit.layout";
        MyGUI::FloatCoord hitFaderCoord(0, 0, 1, 1);
        if (!mResourceSystem->getVFS()->exists(hitFaderTexture))
        {
            hitFaderTexture = "textures\\player_hit_01.dds";
            hitFaderCoord = MyGUI::FloatCoord(0.2f, 0.25f, 0.6f, 0.5f);
        }
        auto hitFader = std::make_unique<ScreenFader>(hitFaderTexture, hitFaderLayout, hitFaderCoord);
        mHitFader = hitFader.get();
        mWindows.push_back(std::move(hitFader));

        auto screenFader = std::make_unique<ScreenFader>("black");
        mScreenFader = screenFader.get();
        mWindows.push_back(std::move(screenFader));

        auto debugWindow = std::make_unique<DebugWindow>();
        mDebugWindow = debugWindow.get();
        mWindows.push_back(std::move(debugWindow));
        trackWindow(mDebugWindow, makeDebugWindowSettingValues());

        auto postProcessorHud = std::make_unique<PostProcessorHud>(mCfgMgr);
        mPostProcessorHud = postProcessorHud.get();
        mWindows.push_back(std::move(postProcessorHud));
        trackWindow(mPostProcessorHud, makePostprocessorWindowSettingValues());

        auto controllerButtonsOverlay = std::make_unique<ControllerButtonsOverlay>();
        mControllerButtonsOverlay = controllerButtonsOverlay.get();
        mWindows.push_back(std::move(controllerButtonsOverlay));

        auto inventoryTabsOverlay = std::make_unique<InventoryTabsOverlay>();
        mInventoryTabsOverlay = inventoryTabsOverlay.get();
        mWindows.push_back(std::move(inventoryTabsOverlay));

        mControllerTooltipEnabled = Settings::gui().mControllerTooltips;
        mActiveControllerWindows[GM_Inventory] = 1; // Start on Inventory page

        mInputBlocker = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
            {}, 0, 0, w, h, MyGUI::Align::Stretch, "InputBlocker");

        mHud->setVisible(true);

        mCharGen = std::make_unique<CharacterCreation>(mViewer->getSceneData()->asGroup(), mResourceSystem);

        updatePinnedWindows();

        // Set up visibility
        updateVisible();

        mStatsWatcher->addListener(mHud);
        mStatsWatcher->addListener(mStatsWindow);
        mStatsWatcher->addListener(mCharGen.get());

        for (auto& window : mWindows)
        {
            std::string_view id = window->getWindowIdForLua();
            if (!id.empty())
                mLuaIdToWindow.emplace(id, window.get());
        }
    }

    void WindowManager::setNewGame(bool newgame)
    {
        if (newgame)
        {
            disallowAll();

            mStatsWatcher->removeListener(mCharGen.get());
            mCharGen = std::make_unique<CharacterCreation>(mViewer->getSceneData()->asGroup(), mResourceSystem);
            mStatsWatcher->addListener(mCharGen.get());
        }
        else
            allow(GW_ALL);

        mStatsWatcher->forceUpdate();
    }

    WindowManager::~WindowManager()
    {
        try
        {
            LuaUi::clearGameInterface();
            LuaUi::clearMenuInterface();

            mStatsWatcher.reset();

            MyGUI::LanguageManager::getInstance().eventRequestTag.clear();
            MyGUI::PointerManager::getInstance().eventChangeMousePointer.clear();
            MyGUI::InputManager::getInstance().eventChangeKeyFocus.clear();
            MyGUI::ClipboardManager::getInstance().eventClipboardChanged.clear();
            MyGUI::ClipboardManager::getInstance().eventClipboardRequested.clear();

            mWindows.clear();
            mMessageBoxManager.reset();
            mToolTips.reset();
            mCharGen.reset();

            mKeyboardNavigation.reset();

            cleanupGarbage();

            mFontLoader.reset();

            mGui->shutdown();

            mGuiPlatform->shutdown();
        }
        catch (const MyGUI::Exception& e)
        {
            Log(Debug::Error) << "Error in the destructor: " << e.what();
        }
    }

    void WindowManager::setStore(const MWWorld::ESMStore& store)
    {
        mStore = &store;
    }

    void WindowManager::cleanupGarbage()
    {
        // Delete any dialogs which are no longer in use
        mGarbageDialogs.clear();
    }

    void WindowManager::enableScene(bool enable)
    {
        unsigned int disablemask = MWRender::Mask_GUI | MWRender::Mask_PreCompile;
        if (!enable && getCullMask() != disablemask)
        {
            mOldUpdateMask = mViewer->getUpdateVisitor()->getTraversalMask();
            mOldCullMask = getCullMask();
            mViewer->getUpdateVisitor()->setTraversalMask(disablemask);
            setCullMask(disablemask);
        }
        else if (enable && getCullMask() == disablemask)
        {
            mViewer->getUpdateVisitor()->setTraversalMask(mOldUpdateMask);
            setCullMask(mOldCullMask);
        }
    }

    void WindowManager::updateConsoleObjectPtr(const MWWorld::Ptr& currentPtr, const MWWorld::Ptr& newPtr)
    {
        mConsole->updateSelectedObjectPtr(currentPtr, newPtr);
    }

    void WindowManager::updateVisible()
    {
        bool loading = (getMode() == GM_Loading || getMode() == GM_LoadingWallpaper);

        bool mainmenucover = containsMode(GM_MainMenu)
            && MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame;

        enableScene(!loading && !mainmenucover);

        if (!mMap)
            return; // UI not created yet

        mHud->setVisible(mHudEnabled && !loading);
        mToolTips->setVisible(mHudEnabled && !loading);

        bool gameMode = !isGuiMode();

        MWBase::Environment::get().getInputManager()->changeInputMode(!gameMode);

        mInputBlocker->setVisible(gameMode);

        if (loading)
            setCursorVisible(mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox());
        else
            setCursorVisible(!gameMode);

        if (gameMode)
            setKeyFocusWidget(nullptr);

        // Icons of forced hidden windows are displayed
        setMinimapVisibility((mAllowed & GW_Map) && (!mMap->pinned() || (mForceHidden & GW_Map)));
        setWeaponVisibility(
            (mAllowed & GW_Inventory) && (!mInventoryWindow->pinned() || (mForceHidden & GW_Inventory)));
        setSpellVisibility((mAllowed & GW_Magic) && (!mSpellWindow->pinned() || (mForceHidden & GW_Magic)));
        setHMSVisibility((mAllowed & GW_Stats) && (!mStatsWindow->pinned() || (mForceHidden & GW_Stats)));

        mInventoryWindow->setGuiMode(getMode());

        // If in game mode (or interactive messagebox), show the pinned windows
        if (mGuiModes.empty())
        {
            mMap->setVisible(mMap->pinned() && !isConsoleMode() && !(mForceHidden & GW_Map) && (mAllowed & GW_Map));
            mStatsWindow->setVisible(
                mStatsWindow->pinned() && !isConsoleMode() && !(mForceHidden & GW_Stats) && (mAllowed & GW_Stats));
            mInventoryWindow->setVisible(mInventoryWindow->pinned() && !isConsoleMode()
                && !(mForceHidden & GW_Inventory) && (mAllowed & GW_Inventory));
            mSpellWindow->setVisible(
                mSpellWindow->pinned() && !isConsoleMode() && !(mForceHidden & GW_Magic) && (mAllowed & GW_Magic));

            if (Settings::gui().mControllerMenus)
            {
                if (mControllerButtonsOverlay)
                    mControllerButtonsOverlay->setVisible(false);
                if (mInventoryTabsOverlay)
                    mInventoryTabsOverlay->setVisible(false);
            }
            return;
        }
        else if (getMode() != GM_Inventory)
        {
            mMap->setVisible(false);
            mStatsWindow->setVisible(false);
            mSpellWindow->setVisible(false);
            mHud->setDrowningBarVisible(false);
            mInventoryWindow->setVisible(
                getMode() == GM_Container || getMode() == GM_Barter || getMode() == GM_Companion);
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

        updateControllerButtonsOverlay();

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

    void WindowManager::setDrowningTimeLeft(float time, float maxTime)
    {
        mHud->setDrowningTimeLeft(time, maxTime);
    }

    void WindowManager::removeDialog(std::unique_ptr<Layout>&& dialog)
    {
        if (!dialog)
            return;
        dialog->setVisible(false);
        mGarbageDialogs.push_back(std::move(dialog));
        updateControllerButtonsOverlay();
    }

    void WindowManager::exitCurrentGuiMode()
    {
        if (mDragAndDrop && mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->finish();
            return;
        }

        if (mGuiModes.empty())
            return;

        GuiModeState& state = mGuiModeStates[mGuiModes.back()];
        for (const auto& window : state.mWindows)
        {
            if (!window->exit())
            {
                // unable to exit window, but give access to main menu
                if (!MyGUI::InputManager::getInstance().isModalAny() && getMode() != GM_MainMenu)
                    pushGuiMode(GM_MainMenu);
                return;
            }
        }

        popGuiMode();
    }

    void WindowManager::interactiveMessageBox(
        std::string_view message, const std::vector<std::string>& buttons, bool block, int defaultFocus)
    {
        mMessageBoxManager->createInteractiveMessageBox(message, buttons, block, defaultFocus);
        updateVisible();

        if (block)
        {
            Misc::FrameRateLimiter frameRateLimiter
                = Misc::makeFrameRateLimiter(MWBase::Environment::get().getFrameRateLimit());
            while (mMessageBoxManager->readPressedButton(false) == -1
                && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
            {
                const float dt
                    = std::chrono::duration_cast<std::chrono::duration<float>>(frameRateLimiter.getLastFrameDuration())
                          .count();

                mKeyboardNavigation->onFrame();
                mMessageBoxManager->onFrame(dt);
                MWBase::Environment::get().getInputManager()->update(dt, true, false);

                if (!mWindowVisible)
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
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

                frameRateLimiter.limit();
            }

            mMessageBoxManager->resetInteractiveMessageBox();
        }
    }

    void WindowManager::messageBox(std::string_view message, enum MWGui::ShowInDialogueMode showInDialogueMode)
    {
        if (getMode() == GM_Dialogue && showInDialogueMode != MWGui::ShowInDialogueMode_Never)
        {
            MyGUI::UString text = MyGUI::LanguageManager::getInstance().replaceTags(MyGUI::UString(message));
            mDialogueWindow->addMessageBox(text);
        }
        else if (showInDialogueMode != MWGui::ShowInDialogueMode_Only)
        {
            mMessageBoxManager->createMessageBox(message);
        }
    }

    void WindowManager::scheduleMessageBox(std::string message, enum MWGui::ShowInDialogueMode showInDialogueMode)
    {
        mScheduledMessageBoxes.lock()->emplace_back(std::move(message), showInDialogueMode);
    }

    void WindowManager::staticMessageBox(std::string_view message)
    {
        mMessageBoxManager->createMessageBox(message, true);
    }

    void WindowManager::removeStaticMessageBox()
    {
        mMessageBoxManager->removeStaticMessageBox();
    }

    int WindowManager::readPressedButton()
    {
        return mMessageBoxManager->readPressedButton();
    }

    std::string_view WindowManager::getGameSettingString(std::string_view id, std::string_view defaultValue)
    {
        const ESM::GameSetting* setting = mStore->get<ESM::GameSetting>().search(id);

        if (setting && setting->mValue.getType() == ESM::VT_String)
            return setting->mValue.getString();

        return defaultValue;
    }

    void WindowManager::updateMap()
    {
        if (!mLocalMapRender)
            return;

        MWWorld::ConstPtr player = MWMechanics::getPlayer();

        osg::Vec3f playerPosition = player.getRefData().getPosition().asVec3();
        osg::Quat playerOrientation(-player.getRefData().getPosition().rot[2], osg::Vec3(0, 0, 1));

        osg::Vec3f playerdirection;
        int x, y;
        float u, v;
        mLocalMapRender->updatePlayer(playerPosition, playerOrientation, u, v, x, y, playerdirection);

        if (!player.getCell()->isExterior())
        {
            setActiveMap(*player.getCell()->getCell());
        }
        // else: need to know the current grid center, call setActiveMap from changeCell

        mMap->setPlayerDir(playerdirection.x(), playerdirection.y());
        mMap->setPlayerPos(x, y, u, v);
        mHud->setPlayerDir(playerdirection.x(), playerdirection.y());
        mHud->setPlayerPos(x, y, u, v);
    }

    WindowBase* WindowManager::getActiveControllerWindow()
    {
        if (!mCurrentModals.empty())
            return mCurrentModals.back();

        if (mWindows.empty())
            return nullptr;

        if (isSettingsWindowVisible())
            return mSettingsWindow;

        if (!mGuiModes.empty())
        {
            GuiMode mode = mGuiModes.back();
            GuiModeState& state = mGuiModeStates[mode];
            if (state.mWindows.empty())
                return nullptr;

            size_t activeIndex = std::clamp<size_t>(mActiveControllerWindows[mode], 0, state.mWindows.size() - 1);

            // If the active window is no longer visible, find the next visible window.
            if (!state.mWindows[activeIndex]->isVisible())
                cycleActiveControllerWindow(true);

            return state.mWindows[activeIndex];
        }

        return nullptr;
    }

    void WindowManager::cycleActiveControllerWindow(bool next)
    {
        if (!Settings::gui().mControllerMenus || mGuiModes.empty())
            return;

        GuiMode mode = mGuiModes.back();
        size_t winCount = mGuiModeStates[mode].mWindows.size();

        size_t activeIndex = 0;
        if (winCount > 1)
        {
            // Find next/previous visible window
            activeIndex = mActiveControllerWindows[mode];
            int delta = next ? 1 : -1;

            for (size_t i = 0; i < winCount; ++i)
            {
                activeIndex = wrap(activeIndex, winCount, delta);
                if (mGuiModeStates[mode].mWindows[activeIndex]->isVisible())
                    break;
            }
        }

        if (mActiveControllerWindows[mode] != activeIndex)
            setActiveControllerWindow(mode, activeIndex);
    }

    void WindowManager::reapplyActiveControllerWindow()
    {
        if (!Settings::gui().mControllerMenus || mGuiModes.empty())
            return;

        const GuiMode mode = mGuiModes.back();
        size_t winCount = mGuiModeStates[mode].mWindows.size();

        for (size_t i = 0; i < winCount; i++)
        {
            // Set active window last so inactive windows don't stomp on changes it makes, e.g. to tooltips.
            if (i != mActiveControllerWindows[mode])
                mGuiModeStates[mode].mWindows[i]->setActiveControllerWindow(false);
        }
        if (winCount > 0)
            mGuiModeStates[mode].mWindows[mActiveControllerWindows[mode]]->setActiveControllerWindow(true);
    }

    void WindowManager::setActiveControllerWindow(GuiMode mode, size_t activeIndex)
    {
        if (!Settings::gui().mControllerMenus)
            return;

        size_t winCount = mGuiModeStates[mode].mWindows.size();
        if (winCount == 0)
            return;

        activeIndex = std::clamp<size_t>(activeIndex, 0, winCount - 1);
        mActiveControllerWindows[mode] = activeIndex;

        reapplyActiveControllerWindow();

        MWBase::Environment::get().getInputManager()->setGamepadGuiCursorEnabled(
            mGuiModeStates[mode].mWindows[activeIndex]->isGamepadCursorAllowed());

        updateControllerButtonsOverlay();
        setCursorActive(false);

        if (winCount > 1)
            playSound(ESM::RefId::stringRefId("Menu Size"));
    }

    void WindowManager::update(float frameDuration)
    {
        handleScheduledMessageBoxes();

        bool gameRunning
            = MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_NoGame;

        if (gameRunning)
            updateMap();

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
        if (mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox()
            && mCurrentModals.back() != mMessageBoxManager->getInteractiveMessageBox())
        {
            std::vector<WindowModal*>::iterator found = std::find(
                mCurrentModals.begin(), mCurrentModals.end(), mMessageBoxManager->getInteractiveMessageBox());
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

        if (mMessageBoxManager)
            mMessageBoxManager->onFrame(frameDuration);

        mToolTips->onFrame(frameDuration);

        if (mLocalMapRender)
            //mLocalMapRender->cleanupCameras();

        mDebugWindow->onFrame(frameDuration);

        if (isConsoleMode())
            mConsole->onFrame(frameDuration);

        if (isSettingsWindowVisible())
            mSettingsWindow->onFrame(frameDuration);

        if (mControllerButtonsOverlay && mControllerButtonsOverlay->isVisible())
            mControllerButtonsOverlay->onFrame(frameDuration);

        if (mInventoryTabsOverlay && mInventoryTabsOverlay->isVisible())
            mInventoryTabsOverlay->onFrame(frameDuration);

        if (!gameRunning)
            return;

        // We should display message about crime only once per frame, even if there are several crimes.
        // Otherwise we will get message spam when stealing several items via Take All button.
        const MWWorld::Ptr player = MWMechanics::getPlayer();
        const MWWorld::Class& playerCls = player.getClass();
        int currentBounty = playerCls.getNpcStats(player).getBounty();
        if (currentBounty != mPlayerBounty)
        {
            if (mPlayerBounty >= 0 && currentBounty > mPlayerBounty)
                messageBox("#{sCrimeMessage}");

            mPlayerBounty = currentBounty;
        }

        MWBase::LuaManager::ActorControls* playerControls
            = MWBase::Environment::get().getLuaManager()->getActorControls(player);
        bool triedToMove = playerControls
            && (playerControls->mMovement != 0 || playerControls->mSideMovement != 0 || playerControls->mJump);
        if (mMessageBoxManager && triedToMove && playerCls.getEncumbrance(player) > playerCls.getCapacity(player))
        {
            const auto& msgboxs = mMessageBoxManager->getActiveMessageBoxes();
            auto it
                = std::find_if(msgboxs.begin(), msgboxs.end(), [](const std::unique_ptr<MWGui::MessageBox>& msgbox) {
                      return (msgbox->getMessage() == "#{sNotifyMessage59}");
                  });

            // if an overencumbered messagebox is already present, reset its expiry timer,
            // otherwise create a new one.
            if (it != msgboxs.end())
                (*it)->mCurrentTime = 0;
            else
                messageBox("#{sNotifyMessage59}");
        }

        mDragAndDrop->onFrame();

        mHud->onFrame(frameDuration);

        mPostProcessorHud->onFrame(frameDuration);

        if (mCharGen)
            mCharGen->onFrame(frameDuration);

        updateActivatedQuickKey();

        mStatsWatcher->update();

        cleanupGarbage();
    }

    void WindowManager::changeCell(const MWWorld::CellStore* cell)
    {
        mMap->requestMapRender(cell);

        std::string name{ MWBase::Environment::get().getWorld()->getCellName(cell) };

        mMap->setCellName(name);
        mHud->setCellName(name);
        auto cellCommon = cell->getCell();

        if (cellCommon->isExterior())
        {
            if (!cellCommon->getNameId().empty())
                mMap->addVisitedLocation(name, cellCommon->getGridX(), cellCommon->getGridY());

            mMap->cellExplored(cellCommon->getGridX(), cellCommon->getGridY());
        }
        else
        {
            osg::Vec3f worldPos;
            if (!MWBase::Environment::get().getWorld()->findInteriorPositionInWorldSpace(cell, worldPos))
                worldPos = MWBase::Environment::get().getWorld()->getPlayer().getLastKnownExteriorPosition();
            else
                MWBase::Environment::get().getWorld()->getPlayer().setLastKnownExteriorPosition(worldPos);
            mMap->setGlobalMapPlayerPosition(worldPos.x(), worldPos.y());
        }
        setActiveMap(*cellCommon);
    }

    void WindowManager::setActiveMap(const MWWorld::Cell& cell)
    {
        mMap->setActiveCell(cell);
        mHud->setActiveCell(cell);
    }

    void WindowManager::setDrowningBarVisibility(bool visible)
    {
        mHud->setDrowningBarVisible(visible);
    }

    void WindowManager::setHMSVisibility(bool visible)
    {
        mHud->setHmsVisible(visible);
    }

    void WindowManager::setMinimapVisibility(bool visible)
    {
        mHud->setMinimapVisible(visible);
    }

    bool WindowManager::toggleFogOfWar()
    {
        mMap->toggleFogOfWar();
        return mHud->toggleFogOfWar();
    }

    void WindowManager::setFocusObject(const MWWorld::Ptr& focus)
    {
        mToolTips->setFocusObject(focus);

        const int showOwned = Settings::game().mShowOwned;
        if (mHud && (showOwned == 2 || showOwned == 3))
        {
            bool owned = mToolTips->checkOwned();
            mHud->setCrosshairOwned(owned);
        }
    }

    void WindowManager::setFocusObjectScreenCoords(float x, float y)
    {
        mToolTips->setFocusObjectScreenCoords(x, y);
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
        mHud->setWeapVisible(visible);
    }

    void WindowManager::setSpellVisibility(bool visible)
    {
        mHud->setSpellVisible(visible);
        mHud->setEffectVisible(visible);
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

    void WindowManager::setCursorActive(bool active)
    {
        mCursorActive = active;
    }

    void WindowManager::onRetrieveTag(const MyGUI::UString& tag, MyGUI::UString& result)
    {
        std::string_view tagView = tag;

        constexpr std::string_view myGuiPrefix = "setting=";

        constexpr std::string_view tokenToFind = "sCell=";

        if (tagView.starts_with(myGuiPrefix))
        {
            tagView = tagView.substr(myGuiPrefix.length());
            const size_t commaPos = tagView.find(',');
            if (commaPos == std::string_view::npos)
                throw std::runtime_error("Invalid setting tag (expected comma): " + std::string(tagView));

            std::string_view settingSection = tagView.substr(0, commaPos);
            std::string_view settingTag = tagView.substr(commaPos + 1, tagView.length());

            result = Settings::get<MyGUI::Colour>(settingSection, settingTag).get().print();
        }
        else if (tagView.starts_with(tokenToFind))
        {
            std::string_view cellName = mTranslationDataStorage.translateCellName(tagView.substr(tokenToFind.length()));
            result.assign(cellName.data(), cellName.size());
            result = MyGUI::TextIterator::toTagsString(result);
        }
        else if (Gui::replaceTag(tagView, result))
        {
            return;
        }
        else
        {
            std::vector<std::string> split;
            Misc::StringUtils::split(tagView, split, ":");

            L10n::Manager& l10nManager = *MWBase::Environment::get().getL10nManager();

            // If a key has a "Context:KeyName" format, use YAML to translate data
            if (split.size() == 2)
            {
                result = l10nManager.getContext(split[0])->formatMessage(split[1], {}, {});
                return;
            }

            // If not, treat is as GMST name from legacy localization
            if (!mStore)
            {
                Log(Debug::Error) << "Error: WindowManager::onRetrieveTag: no Store set up yet, can not replace '"
                                  << tagView << "'";
                result.assign(tagView.data(), tagView.size());
                return;
            }
            const ESM::GameSetting* setting = mStore->get<ESM::GameSetting>().search(tagView);

            if (setting && setting->mValue.getType() == ESM::VT_String)
                result = setting->mValue.getString();
            else
                result.assign(tagView.data(), tagView.size());
        }
    }

    void WindowManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        bool changeRes = false;
        for (const auto& setting : changed)
        {
            if (setting.first == "GUI" && setting.second == "menu transparency")
                setMenuTransparency(Settings::gui().mMenuTransparency);
            else if (setting.first == "Video"
                && (setting.second == "resolution x" || setting.second == "resolution y"
                    || setting.second == "window mode" || setting.second == "window border"))
                changeRes = true;

            else if (setting.first == "Video" && setting.second == "vsync mode")
                mVideoWrapper->setSyncToVBlank(Settings::video().mVsyncMode);
            else if (setting.first == "Video" && (setting.second == "gamma" || setting.second == "contrast"))
                mVideoWrapper->setGammaContrast(Settings::video().mGamma, Settings::video().mContrast);
        }

        if (changeRes)
        {
            mVideoWrapper->setVideoMode(Settings::video().mResolutionX, Settings::video().mResolutionY,
                Settings::video().mWindowMode, Settings::video().mWindowBorder);
        }
    }

    void WindowManager::windowResized(int x, int y)
    {
        Settings::video().mResolutionX.set(x);
        Settings::video().mResolutionY.set(y);

        // We only want to process changes to window-size related settings.
        Settings::CategorySettingVector filter = { { "Video", "resolution x" }, { "Video", "resolution y" } };

        // If the HUD has not been initialised, the World singleton will not be available.
        if (mHud)
        {
            MWBase::Environment::get().getWorld()->processChangedSettings(Settings::Manager::getPendingChanges(filter));
        }

        Settings::Manager::resetPendingChanges(filter);

        mGuiPlatform->getRenderManagerPtr()->setViewSize(x, y);

        // scaled size
        const MyGUI::IntSize& viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        x = viewSize.width;
        y = viewSize.height;

        sizeVideo(x, y);

        if (!mHud)
            return; // UI not initialized yet

        for (const auto& [window, settings] : mTrackedWindows)
        {
            const WindowRectSettingValues& rect = settings.mIsMaximized ? settings.mMaximized : settings.mRegular;
            window->setPosition(MyGUI::IntPoint(static_cast<int>(rect.mX * x), static_cast<int>(rect.mY * y)));
            window->setSize(MyGUI::IntSize(static_cast<int>(rect.mW * x), static_cast<int>(rect.mH * y)));

            WindowBase::clampWindowCoordinates(window);
        }

        for (const auto& window : mWindows)
            window->onResChange(x, y);

        // Re-apply any controller-specific window changes.
        reapplyActiveControllerWindow();

        // TODO: check if any windows are now off-screen and move them back if so
    }

    bool WindowManager::isWindowVisible() const
    {
        return mWindowVisible;
    }

    void WindowManager::windowVisibilityChange(bool visible)
    {
        mWindowVisible = visible;
    }

    void WindowManager::windowClosed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void WindowManager::onCursorChange(std::string_view name)
    {
        mCursorManager->cursorChanged(name);
    }

    void WindowManager::pushGuiMode(GuiMode mode)
    {
        pushGuiMode(mode, MWWorld::Ptr());
    }

    void WindowManager::pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg)
    {
        pushGuiMode(mode, arg, false);
    }

    void WindowManager::forceLootMode(const MWWorld::Ptr& ptr)
    {
        pushGuiMode(MWGui::GM_Container, ptr, true);
    }

    void WindowManager::pushGuiMode(GuiMode mode, const MWWorld::Ptr& arg, bool force)
    {
        if (mode == GM_Inventory && mAllowed == GW_None)
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
        }
        if (force)
            mContainerWindow->treatNextOpenAsLoot();

        try
        {
            for (WindowBase* window : mGuiModeStates[mode].mWindows)
                window->setPtr(arg);
        }
        catch (...)
        {
            popGuiMode();
            throw;
        }

        mKeyboardNavigation->restoreFocus(mode);

        updateVisible();
        MWBase::Environment::get().getLuaManager()->uiModeChanged(arg);

        if (Settings::gui().mControllerMenus)
        {
            if (mode == GM_Container)
                mActiveControllerWindows[mode] = 0; // Ensure controller focus is on container
            // Activate first visible window. This needs to be called after updateVisible.
            if (mActiveControllerWindows[mode] != 0)
                mActiveControllerWindows[mode] = mActiveControllerWindows[mode] - 1;
            cycleActiveControllerWindow(true);
        }
    }

    void WindowManager::setCullMask(uint32_t mask)
    {
        mViewer->getCamera()->setCullMask(mask);

        // We could check whether stereo is enabled here, but these methods are
        // trivial and have no effect in mono or multiview so just call them regardless.
        mViewer->getCamera()->setCullMaskLeft(mask);
        mViewer->getCamera()->setCullMaskRight(mask);
    }

    uint32_t WindowManager::getCullMask()
    {
        return mViewer->getCamera()->getCullMask();
    }

    void WindowManager::popGuiMode(bool forceExit)
    {
        if (mDragAndDrop && mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->finish();
        }

        if (!mGuiModes.empty())
        {
            const GuiMode mode = mGuiModes.back();
            if (forceExit)
            {
                GuiModeState& state = mGuiModeStates[mode];
                for (const auto& window : state.mWindows)
                    window->exit();
            }
            mKeyboardNavigation->saveFocus(mode);
            if (containsMode(mode))
            {
                mGuiModes.pop_back();
                mGuiModeStates[mode].update(false);
                MWBase::Environment::get().getLuaManager()->uiModeChanged(MWWorld::Ptr());
            }
        }

        if (!mGuiModes.empty())
        {
            const GuiMode mode = mGuiModes.back();
            mGuiModeStates[mode].update(true);
            mKeyboardNavigation->restoreFocus(mode);
        }

        updateVisible();

        // To make sure that console window get focus again
        if (mConsole && mConsole->isVisible())
            mConsole->onOpen();

        if (Settings::gui().mControllerMenus)
        {
            if (mGuiModes.empty())
            {
                setControllerTooltipVisible(false);
                // When all windows are hidden, reset tooltip visibility to user's preference.
                mControllerTooltipEnabled = Settings::gui().mControllerTooltips;
            }
            else
                reapplyActiveControllerWindow();
        }
    }

    void WindowManager::removeGuiMode(GuiMode mode)
    {
        if (!mGuiModes.empty() && mGuiModes.back() == mode)
        {
            popGuiMode();
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
        MWBase::Environment::get().getLuaManager()->uiModeChanged(MWWorld::Ptr());
    }

    void WindowManager::goToJail(int days)
    {
        pushGuiMode(MWGui::GM_Jail);
        mJailScreen->goToJail(days);
    }

    void WindowManager::setSelectedSpell(const ESM::RefId& spellId, int successChancePercent)
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
        mSelectedSpell = ESM::RefId();
        const ESM::Enchantment* ench = mStore->get<ESM::Enchantment>().find(item.getClass().getEnchantment(item));

        int chargePercent = static_cast<int>(item.getCellRef().getNormalizedEnchantmentCharge(*ench) * 100);
        mHud->setSelectedEnchantItem(item, chargePercent);
        mSpellWindow->setTitle(item.getClass().getName(item));
    }

    const MWWorld::Ptr& WindowManager::getSelectedEnchantItem() const
    {
        return mSelectedEnchantItem;
    }

    void WindowManager::setSelectedWeapon(const MWWorld::Ptr& item)
    {
        mSelectedWeapon = item;
        int durabilityPercent = 100;
        if (item.getClass().hasItemHealth(item))
        {
            durabilityPercent = static_cast<int>(item.getClass().getItemNormalizedHealth(item) * 100);
        }
        mHud->setSelectedWeapon(item, durabilityPercent);
        mInventoryWindow->setTitle(item.getClass().getName(item));
    }

    const MWWorld::Ptr& WindowManager::getSelectedWeapon() const
    {
        return mSelectedWeapon;
    }

    void WindowManager::unsetSelectedSpell()
    {
        mSelectedSpell = ESM::RefId();
        mSelectedEnchantItem = MWWorld::Ptr();
        mHud->unsetSelectedSpell();

        MWWorld::Player* player = &MWBase::Environment::get().getWorld()->getPlayer();
        if (player->getDrawState() == MWMechanics::DrawState::Spell)
            player->setDrawState(MWMechanics::DrawState::Nothing);

        mSpellWindow->setTitle("#{Interface:None}");
    }

    void WindowManager::unsetSelectedWeapon()
    {
        mSelectedWeapon = MWWorld::Ptr();
        mHud->unsetSelectedWeapon();
        mInventoryWindow->setTitle("#{sSkillHandtohand}");
    }

    void WindowManager::getMousePosition(int& x, int& y)
    {
        const MyGUI::IntPoint& pos = MyGUI::InputManager::getInstance().getMousePosition();
        x = pos.left;
        y = pos.top;
    }

    void WindowManager::getMousePosition(float& x, float& y)
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

    float WindowManager::getScalingFactor() const
    {
        return mScalingFactor;
    }

    void WindowManager::executeInConsole(const std::filesystem::path& path)
    {
        mConsole->executeFile(path);
    }

    std::vector<MWGui::WindowBase*> WindowManager::getGuiModeWindows(GuiMode mode)
    {
        return mGuiModeStates[mode].mWindows;
    }
    MWGui::InventoryWindow* WindowManager::getInventoryWindow()
    {
        return mInventoryWindow;
    }
    MWGui::CountDialog* WindowManager::getCountDialog()
    {
        return mCountDialog;
    }
    MWGui::ConfirmationDialog* WindowManager::getConfirmationDialog()
    {
        return mConfirmationDialog;
    }
    MWGui::HUD* WindowManager::getHud()
    {
        return mHud;
    }
    MWGui::TradeWindow* WindowManager::getTradeWindow()
    {
        return mTradeWindow;
    }
    MWGui::PostProcessorHud* WindowManager::getPostProcessorHud()
    {
        return mPostProcessorHud;
    }

    void WindowManager::useItem(const MWWorld::Ptr& item, bool bypassBeastRestrictions)
    {
        if (mInventoryWindow)
            mInventoryWindow->useItem(item, bypassBeastRestrictions);
    }

    bool WindowManager::isAllowed(GuiWindow wnd) const
    {
        return (mAllowed & wnd) != 0;
    }

    void WindowManager::allow(GuiWindow wnd)
    {
        mAllowed = (GuiWindow)(mAllowed | wnd);

        if (wnd & GW_Inventory)
        {
            mBookWindow->setInventoryAllowed(true);
            mScrollWindow->setInventoryAllowed(true);
        }

        updateVisible();
    }

    void WindowManager::disallowAll()
    {
        mAllowed = GW_None;
        mRestAllowed = false;

        mBookWindow->setInventoryAllowed(false);
        mScrollWindow->setInventoryAllowed(false);

        updateVisible();
    }

    void WindowManager::toggleVisible(GuiWindow wnd)
    {
        if (getMode() != GM_Inventory)
            return;

        if (Settings::SettingValue<bool>* const hidden = findHiddenSetting(wnd))
            hidden->set(!hidden->get());

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
        return !mGuiModes.empty() || isConsoleMode() || isPostProcessorHudVisible() || isInteractiveMessageBoxActive();
    }

    bool WindowManager::isConsoleMode() const
    {
        return mConsole && mConsole->isVisible();
    }

    bool WindowManager::isPostProcessorHudVisible() const
    {
        return mPostProcessorHud && mPostProcessorHud->isVisible();
    }

    bool WindowManager::isSettingsWindowVisible() const
    {
        return mSettingsWindow && mSettingsWindow->isVisible();
    }

    bool WindowManager::isInteractiveMessageBoxActive() const
    {
        return mMessageBoxManager && mMessageBoxManager->isInteractiveMessageBox();
    }

    MWGui::GuiMode WindowManager::getMode() const
    {
        if (mGuiModes.empty())
            return GM_None;
        return mGuiModes.back();
    }

    void WindowManager::disallowMouse()
    {
        mInputBlocker->setVisible(true);
    }

    void WindowManager::allowMouse()
    {
        mInputBlocker->setVisible(!isGuiMode());
    }

    void WindowManager::notifyInputActionBound()
    {
        mSettingsWindow->updateControlsBox();
        allowMouse();
    }

    bool WindowManager::containsMode(GuiMode mode) const
    {
        if (mGuiModes.empty())
            return false;

        return std::find(mGuiModes.begin(), mGuiModes.end(), mode) != mGuiModes.end();
    }

    void WindowManager::showCrosshair(bool show)
    {
        if (mHud)
            mHud->setCrosshairVisible(show && Settings::hud().mCrosshair);
    }

    void WindowManager::updateActivatedQuickKey()
    {
        mQuickKeysMenu->updateActivatedQuickKey();
    }

    void WindowManager::activateQuickKey(int index)
    {
        mQuickKeysMenu->activateQuickKey(index);
    }

    bool WindowManager::setHudVisibility(bool show)
    {
        mHudEnabled = show;
        updateVisible();
        mMessageBoxManager->setVisible(mHudEnabled);
        return mHudEnabled;
    }

    bool WindowManager::getRestEnabled()
    {
        // Enable rest dialogue if character creation finished
        if (mRestAllowed == false
            && MWBase::Environment::get().getWorld()->getGlobalFloat(MWWorld::Globals::sCharGenState) == -1)
            mRestAllowed = true;
        return mRestAllowed;
    }

    bool WindowManager::getPlayerSleeping()
    {
        return mWaitDialog->getSleeping();
    }

    void WindowManager::wakeUpPlayer()
    {
        mWaitDialog->wakeUp();
    }

    void WindowManager::addVisitedLocation(const std::string& name, int x, int y)
    {
        mMap->addVisitedLocation(name, x, y);
    }

    const Translation::Storage& WindowManager::getTranslationDataStorage() const
    {
        return mTranslationDataStorage;
    }

    void WindowManager::changePointer(const std::string& name)
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

    // Remove this wrapper once onKeyFocusChanged call is rendered unnecessary
    void WindowManager::setKeyFocusWidget(MyGUI::Widget* widget)
    {
        MyGUI::InputManager::getInstance().setKeyFocusWidget(widget);
        onKeyFocusChanged(widget);
    }

    void WindowManager::onKeyFocusChanged(MyGUI::Widget* widget)
    {
        bool isEditBox = widget && widget->castType<MyGUI::EditBox>(false);
        LuaUi::WidgetExtension* luaWidget = dynamic_cast<LuaUi::WidgetExtension*>(widget);
        bool capturesInput = luaWidget ? luaWidget->isTextInput() : isEditBox;
        if (widget && capturesInput)
            SDL_StartTextInput();
        else
            SDL_StopTextInput();
    }

    void WindowManager::setEnemy(const MWWorld::Ptr& enemy)
    {
        mHud->setEnemy(enemy);
    }

    std::size_t WindowManager::getMessagesCount() const
    {
        std::size_t count = 0;
        if (mMessageBoxManager)
            count = mMessageBoxManager->getMessagesCount();

        return count;
    }

    Loading::Listener* WindowManager::getLoadingScreen()
    {
        return mLoadingScreen;
    }

    bool WindowManager::getCursorVisible()
    {
        return mCursorVisible && mCursorActive;
    }

    void WindowManager::trackWindow(Layout* layout, const WindowSettingValues& settings)
    {
        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();

        const WindowRectSettingValues& rect = settings.mIsMaximized ? settings.mMaximized : settings.mRegular;

        MyGUI::Window* window = layout->mMainWidget->castType<MyGUI::Window>();
        window->setPosition(
            MyGUI::IntPoint(static_cast<int>(rect.mX * viewSize.width), static_cast<int>(rect.mY * viewSize.height)));
        window->setSize(
            MyGUI::IntSize(static_cast<int>(rect.mW * viewSize.width), static_cast<int>(rect.mH * viewSize.height)));

        window->eventWindowChangeCoord += MyGUI::newDelegate(this, &WindowManager::onWindowChangeCoord);
        WindowBase::clampWindowCoordinates(window);

        mTrackedWindows.emplace(window, settings);
    }

    void WindowManager::toggleMaximized(Layout* layout)
    {
        MyGUI::Window* window = layout->mMainWidget->castType<MyGUI::Window>();
        const auto it = mTrackedWindows.find(window);
        if (it == mTrackedWindows.end())
            return;

        const WindowSettingValues& settings = it->second;
        const WindowRectSettingValues& rect = settings.mIsMaximized ? settings.mRegular : settings.mMaximized;

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        const int x = static_cast<int>(rect.mX * viewSize.width);
        const int y = static_cast<int>(rect.mY * viewSize.height);
        const int w = static_cast<int>(rect.mW * viewSize.width);
        const int h = static_cast<int>(rect.mH * viewSize.height);
        window->setCoord(x, y, w, h);

        settings.mIsMaximized.set(!settings.mIsMaximized.get());
    }

    void WindowManager::onWindowChangeCoord(MyGUI::Window* window)
    {
        // If using controller menus, don't persist changes to size of the stats or magic
        // windows.
        if (Settings::gui().mControllerMenus
            && (window == mStatsWindow->mMainWidget->castType<MyGUI::Window>()
                || window == mSpellWindow->mMainWidget->castType<MyGUI::Window>()))
            return;

        const auto it = mTrackedWindows.find(window);
        if (it == mTrackedWindows.end())
            return;

        WindowBase::clampWindowCoordinates(window);

        const WindowSettingValues& settings = it->second;

        MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        settings.mRegular.mX.set(window->getPosition().left / static_cast<float>(viewSize.width));
        settings.mRegular.mY.set(window->getPosition().top / static_cast<float>(viewSize.height));
        settings.mRegular.mW.set(window->getSize().width / static_cast<float>(viewSize.width));
        settings.mRegular.mH.set(window->getSize().height / static_cast<float>(viewSize.height));

        settings.mIsMaximized.set(false);
    }

    void WindowManager::clear()
    {
        mPlayerBounty = -1;

        for (const auto& window : mWindows)
        {
            window->clear();
            window->setDisabledByLua(false);
        }

        if (mLocalMapRender)
            mLocalMapRender->clear();

        mMessageBoxManager->clear();

        mToolTips->clear();

        mSelectedSpell = ESM::RefId();
        mCustomMarkers.clear();

        mForceHidden = GW_None;
        mRestAllowed = true;

        while (!mGuiModes.empty())
            popGuiMode();

        updateVisible();
    }

    void WindowManager::write(ESM::ESMWriter& writer, Loading::Listener& progress)
    {
        mMap->write(writer, progress);

        mQuickKeysMenu->write(writer);

        if (!mSelectedSpell.empty())
        {
            writer.startRecord(ESM::REC_ASPL);
            writer.writeHNRefId("ID__", mSelectedSpell);
            writer.endRecord(ESM::REC_ASPL);
        }

        for (const auto& [_, marker] : mCustomMarkers)
        {
            writer.startRecord(ESM::REC_MARK);
            marker.save(writer);
            writer.endRecord(ESM::REC_MARK);
        }
    }

    void WindowManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_GMAP)
            mMap->readRecord(reader, type);
        else if (type == ESM::REC_KEYS)
            mQuickKeysMenu->readRecord(reader, type);
        else if (type == ESM::REC_ASPL)
        {
            reader.getSubNameIs("ID__");
            ESM::RefId spell = reader.getRefId();
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

    size_t WindowManager::countSavedGameRecords() const
    {
        return 1 // Global map
            + 1 // QuickKeysMenu
            + mCustomMarkers.size() + (!mSelectedSpell.empty() ? 1 : 0);
    }

    bool WindowManager::isSavingAllowed() const
    {
        return !MyGUI::InputManager::getInstance().isModalAny()
            && !isConsoleMode()
            // TODO: remove this, once we have properly serialized the state of open windows
            && (!isGuiMode() || (mGuiModes.size() == 1 && (getMode() == GM_MainMenu || getMode() == GM_Rest)));
    }

    void WindowManager::playVideo(std::string_view name, bool allowSkipping, bool overrideSounds)
    {
        mVideoWidget->playVideo("video\\" + std::string{ name });

        mVideoWidget->eventKeyButtonPressed.clear();
        mVideoBackground->eventKeyButtonPressed.clear();
        if (allowSkipping)
        {
            mVideoWidget->eventKeyButtonPressed += MyGUI::newDelegate(this, &WindowManager::onVideoKeyPressed);
            mVideoBackground->eventKeyButtonPressed += MyGUI::newDelegate(this, &WindowManager::onVideoKeyPressed);
        }

        enableScene(false);

        MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
        sizeVideo(screenSize.width, screenSize.height);

        MyGUI::Widget* oldKeyFocus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        setKeyFocusWidget(mVideoWidget);

        mVideoBackground->setVisible(true);

        bool cursorWasVisible = mCursorVisible;
        setCursorVisible(false);

        if (overrideSounds && mVideoWidget->hasAudioStream())
            MWBase::Environment::get().getSoundManager()->pauseSounds(
                MWSound::VideoPlayback, ~MWSound::Type::Movie & MWSound::Type::Mask);

        Misc::FrameRateLimiter frameRateLimiter
            = Misc::makeFrameRateLimiter(MWBase::Environment::get().getFrameRateLimit());
        while (mVideoWidget->update() && !MWBase::Environment::get().getStateManager()->hasQuitRequest())
        {
            const float dt
                = std::chrono::duration_cast<std::chrono::duration<float>>(frameRateLimiter.getLastFrameDuration())
                      .count();

            MWBase::Environment::get().getInputManager()->update(dt, true, false);

            if (!mWindowVisible)
            {
                mVideoWidget->pause();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            else
            {
                if (mVideoWidget->isPaused())
                    mVideoWidget->resume();

                mViewer->eventTraversal();
                mViewer->updateTraversal();
                mViewer->renderingTraversals();
            }
            // at the time this function is called we are in the middle of a frame,
            // so out of order calls are necessary to get a correct frameNumber for the next frame.
            // refer to the advance() and frame() order in Engine::go()
            mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

            frameRateLimiter.limit();
        }
        mVideoWidget->stop();

        MWBase::Environment::get().getSoundManager()->resumeSounds(MWSound::VideoPlayback);

        setKeyFocusWidget(oldKeyFocus);

        setCursorVisible(cursorWasVisible);

        // Restore normal rendering
        updateVisible();

        mVideoBackground->setVisible(false);
    }

    void WindowManager::sizeVideo(int screenWidth, int screenHeight)
    {
        // Use black bars to correct aspect ratio
        mVideoBackground->setSize(screenWidth, screenHeight);
        mVideoWidget->autoResize(Settings::gui().mStretchMenuBackground);
    }

    void WindowManager::exitCurrentModal()
    {
        if (!mCurrentModals.empty())
        {
            WindowModal* window = mCurrentModals.back();
            if (!window->exit())
                return;
            window->setVisible(false);
            updateControllerButtonsOverlay();
        }
    }

    void WindowManager::addCurrentModal(WindowModal* input)
    {
        if (mCurrentModals.empty())
            mKeyboardNavigation->saveFocus(getMode());

        mCurrentModals.push_back(input);
        mKeyboardNavigation->restoreFocus(-1);

        mKeyboardNavigation->setModalWindow(input->mMainWidget);
        mKeyboardNavigation->setDefaultFocus(input->mMainWidget, input->getDefaultKeyFocus());

        updateControllerButtonsOverlay();
    }

    void WindowManager::removeCurrentModal(WindowModal* input)
    {
        if (!mCurrentModals.empty())
        {
            if (input == mCurrentModals.back())
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
                    Log(Debug::Warning) << "Warning: can't find modal window " << input;
            }
        }
        if (mCurrentModals.empty())
        {
            mKeyboardNavigation->setModalWindow(nullptr);
            mKeyboardNavigation->restoreFocus(getMode());
        }
        else
            mKeyboardNavigation->setModalWindow(mCurrentModals.back()->mMainWidget);
    }

    void WindowManager::onVideoKeyPressed(MyGUI::Widget* /*sender*/, MyGUI::KeyCode key, MyGUI::Char value)
    {
        if (key == MyGUI::KeyCode::Escape)
            mVideoWidget->stop();
    }

    void WindowManager::updatePinnedWindows()
    {
        if (Settings::gui().mControllerMenus)
        {
            // In controller mode, don't hide any menus and only allow pinning the map.
            mInventoryWindow->setPinned(false);
            mMap->setPinned(Settings::windows().mMapPin);
            mSpellWindow->setPinned(false);
            mStatsWindow->setPinned(false);
            return;
        }

        mInventoryWindow->setPinned(Settings::windows().mInventoryPin);
        if (Settings::windows().mInventoryHidden)
            mShown = (GuiWindow)(mShown ^ GW_Inventory);

        mMap->setPinned(Settings::windows().mMapPin);
        if (Settings::windows().mMapHidden)
            mShown = (GuiWindow)(mShown ^ GW_Map);

        mSpellWindow->setPinned(Settings::windows().mSpellsPin);
        if (Settings::windows().mSpellsHidden)
            mShown = (GuiWindow)(mShown ^ GW_Magic);

        mStatsWindow->setPinned(Settings::windows().mStatsPin);
        if (Settings::windows().mStatsHidden)
            mShown = (GuiWindow)(mShown ^ GW_Stats);
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
        if (!Settings::gui().mHitFader)
            return;

        if (!interrupt && !mHitFader->isEmpty())
            return;

        mHitFader->clearQueue();
        mHitFader->fadeTo(100, 0.0f);
        mHitFader->fadeTo(0, 0.5f);
    }

    void WindowManager::setWerewolfOverlay(bool set)
    {
        if (!Settings::gui().mWerewolfOverlay)
            return;

        if (mWerewolfFader)
            mWerewolfFader->notifyAlphaChanged(set ? 1.0f : 0.0f);
    }

    void WindowManager::onClipboardChanged(std::string_view type, std::string_view data)
    {
        if (type == "Text")
            SDL_SetClipboardText(MyGUI::TextIterator::getOnlyText(MyGUI::UString(data)).asUTF8().c_str());
    }

    void WindowManager::onClipboardRequested(std::string_view type, std::string& data)
    {
        if (type != "Text")
            return;
        char* text = nullptr;
        text = SDL_GetClipboardText();
        if (text)
            data = MyGUI::TextIterator::toTagsString(text);

        SDL_free(text);
    }

    void WindowManager::toggleConsole()
    {
        bool visible = mConsole->isVisible();

        if (!visible && !mGuiModes.empty())
            mKeyboardNavigation->saveFocus(mGuiModes.back());

        mConsole->setVisible(!visible);

        if (visible && !mGuiModes.empty())
            mKeyboardNavigation->restoreFocus(mGuiModes.back());

        updateVisible();
    }

    void WindowManager::toggleDebugWindow()
    {
        mDebugWindow->setVisible(!mDebugWindow->isVisible());
    }

    void WindowManager::togglePostProcessorHud()
    {
        if (!MWBase::Environment::get().getWorld()->getPostProcessor()->isEnabled())
        {
            messageBox("#{OMWEngine:PostProcessingIsNotEnabled}");
            return;
        }

        bool visible = mPostProcessorHud->isVisible();

        if (!visible && !mGuiModes.empty())
            mKeyboardNavigation->saveFocus(mGuiModes.back());

        mPostProcessorHud->setVisible(!visible);

        if (visible && !mGuiModes.empty())
            mKeyboardNavigation->restoreFocus(mGuiModes.back());

        updateVisible();
    }

    void WindowManager::toggleSettingsWindow()
    {
        bool visible = mSettingsWindow->isVisible();

        if (!visible && !mGuiModes.empty())
            mKeyboardNavigation->saveFocus(mGuiModes.back());

        mSettingsWindow->setVisible(!visible);

        if (visible && !mGuiModes.empty())
            mKeyboardNavigation->restoreFocus(mGuiModes.back());

        updateVisible();
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

    void WindowManager::playSound(const ESM::RefId& soundId, float volume, float pitch)
    {
        if (soundId.empty())
            return;

        MWBase::Environment::get().getSoundManager()->playSound(
            soundId, volume, pitch, MWSound::Type::Sfx, MWSound::PlayMode::NoEnvNoScaling);
    }

    void WindowManager::updateSpellWindow()
    {
        if (mSpellWindow)
            mSpellWindow->updateSpells();
    }

    void WindowManager::setConsoleSelectedObject(const MWWorld::Ptr& object)
    {
        mConsole->setSelectedObject(object);
    }

    MWWorld::Ptr WindowManager::getConsoleSelectedObject() const
    {
        return mConsole->getSelectedObject();
    }

    void WindowManager::printToConsole(const std::string& msg, std::string_view color)
    {
        mConsole->print(msg, color);
    }

    void WindowManager::setConsoleMode(std::string_view mode)
    {
        mConsole->setConsoleMode(mode);
    }

    const std::string& WindowManager::getConsoleMode()
    {
        return mConsole->getConsoleMode();
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

            const VFS::Path::Normalized path(imgSetPointer->getImageSet()->getIndexInfo(0, 0).texture);

            osg::ref_ptr<osg::Image> image = mResourceSystem->getImageManager()->getImage(path);

            if (image.valid())
            {
                // everything looks good, send it to the cursor manager
                const Uint8 hotspotX = static_cast<Uint8>(imgSetPointer->getHotSpot().left);
                const Uint8 hotspotY = static_cast<Uint8>(imgSetPointer->getHotSpot().top);
                int rotation = imgSetPointer->getRotation();
                MyGUI::IntSize pointerSize = imgSetPointer->getSize();

                mCursorManager->createCursor(imgSetPointer->getResourceName(), rotation, image, hotspotX, hotspotY,
                    pointerSize.width, pointerSize.height);
            }
        }
    }

    void WindowManager::createTextures()
    {
        {
            MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().createTexture("white");
            tex->createManual(8, 8, MyGUI::TextureUsage::Write, MyGUI::PixelFormat::R8G8B8);
            unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
            for (int x = 0; x < 8; ++x)
                for (int y = 0; y < 8; ++y)
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
            for (int x = 0; x < 8; ++x)
                for (int y = 0; y < 8; ++y)
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
            setMenuTransparency(Settings::gui().mMenuTransparency);
        }
    }

    void WindowManager::setMenuTransparency(float value)
    {
        MyGUI::ITexture* tex = MyGUI::RenderManager::getInstance().getTexture("transparent");
        unsigned char* data = reinterpret_cast<unsigned char*>(tex->lock(MyGUI::TextureUsage::Write));
        for (int x = 0; x < 8; ++x)
            for (int y = 0; y < 8; ++y)
            {
                *(data++) = 255;
                *(data++) = 255;
                *(data++) = 255;
                *(data++) = static_cast<unsigned char>(value * 255);
            }
        tex->unlock();
    }

    void WindowManager::addCell(MWWorld::CellStore* cell)
    {
        mLocalMapRender->addCell(cell);
    }

    void WindowManager::removeCell(MWWorld::CellStore* cell)
    {
        mLocalMapRender->removeCell(cell);
    }

    void WindowManager::writeFog(MWWorld::CellStore* cell)
    {
        mLocalMapRender->saveFogOfWar(cell);
    }

    const MWGui::TextColours& WindowManager::getTextColours()
    {
        return mTextColours;
    }

    bool WindowManager::injectKeyPress(MyGUI::KeyCode key, unsigned int text, bool repeat)
    {
        if (!mKeyboardNavigation->injectKeyPress(key, text, repeat))
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

    bool WindowManager::injectKeyRelease(MyGUI::KeyCode key)
    {
        return MyGUI::InputManager::getInstance().injectKeyRelease(key);
    }

    void WindowManager::GuiModeState::update(bool visible)
    {
        for (const auto& window : mWindows)
            window->setVisible(visible);
    }

    void WindowManager::watchActor(const MWWorld::Ptr& ptr)
    {
        mStatsWatcher->watchActor(ptr);
    }

    MWWorld::Ptr WindowManager::getWatchedActor() const
    {
        return mStatsWatcher->getWatchedActor();
    }

    const std::string& WindowManager::getVersionDescription() const
    {
        return mVersionDescription;
    }

    void WindowManager::handleScheduledMessageBoxes()
    {
        const auto scheduledMessageBoxes = mScheduledMessageBoxes.lock();
        for (const ScheduledMessageBox& v : *scheduledMessageBoxes)
            messageBox(v.mMessage, v.mShowInDialogueMode);
        scheduledMessageBoxes->clear();
    }

    void WindowManager::onDeleteCustomData(const MWWorld::Ptr& ptr)
    {
        for (const auto& window : mWindows)
            window->onDeleteCustomData(ptr);
    }

    void WindowManager::asyncPrepareSaveMap()
    {
        mMap->asyncPrepareSaveMap();
    }

    void WindowManager::setDisabledByLua(std::string_view windowId, bool disabled)
    {
        mLuaIdToWindow.at(windowId)->setDisabledByLua(disabled);
        updateVisible();
    }

    bool WindowManager::isWindowVisible(std::string_view windowId) const
    {
        auto it = mLuaIdToWindow.find(windowId);
        if (it == mLuaIdToWindow.end())
            throw std::logic_error("Invalid window name: " + std::string(windowId));
        return it->second->isVisible();
    }

    std::vector<std::string_view> WindowManager::getAllWindowIds() const
    {
        std::vector<std::string_view> res;
        for (const auto& [id, _] : mLuaIdToWindow)
            res.push_back(id);
        return res;
    }

    std::vector<std::string_view> WindowManager::getAllowedWindowIds(GuiMode mode) const
    {
        std::vector<std::string_view> res;
        if (mode == GM_Inventory)
        {
            if (mAllowed & GW_Map)
                res.push_back(mMap->getWindowIdForLua());
            if (mAllowed & GW_Inventory)
                res.push_back(mInventoryWindow->getWindowIdForLua());
            if (mAllowed & GW_Magic)
                res.push_back(mSpellWindow->getWindowIdForLua());
            if (mAllowed & GW_Stats)
                res.push_back(mStatsWindow->getWindowIdForLua());
        }
        else
        {
            auto it = mGuiModeStates.find(mode);
            if (it != mGuiModeStates.end())
            {
                for (const auto* w : it->second.mWindows)
                    if (!w->getWindowIdForLua().empty())
                        res.push_back(w->getWindowIdForLua());
            }
        }
        return res;
    }

    int WindowManager::getControllerMenuHeight()
    {
        int height = MyGUI::RenderManager::getInstance().getViewSize().height;
        if (mControllerButtonsOverlay != nullptr && mControllerButtonsOverlay->isVisible())
            height -= mControllerButtonsOverlay->getHeight();
        if (mInventoryTabsOverlay != nullptr && mInventoryTabsOverlay->isVisible())
            height -= mInventoryTabsOverlay->getHeight();
        return height;
    }

    void WindowManager::setControllerTooltipVisible(bool visible)
    {
        if (!Settings::gui().mControllerMenus)
            return;

        mControllerTooltipVisible = visible;
    }

    void WindowManager::setControllerTooltipEnabled(bool enabled)
    {
        if (!Settings::gui().mControllerMenus)
            return;

        mControllerTooltipEnabled = enabled;
        // When user toggles the setting, also update visibility
        mControllerTooltipVisible = enabled;
    }

    void WindowManager::restoreControllerTooltips()
    {
        // Restore tooltip visibility if user has them enabled but they were hidden by mouse movement
        if (mControllerTooltipEnabled && !mControllerTooltipVisible)
            setControllerTooltipVisible(true);
    }

    void WindowManager::updateControllerButtonsOverlay()
    {
        if (!Settings::gui().mControllerMenus || !mControllerButtonsOverlay)
            return;

        if (mWindows.empty())
            return;

        WindowBase* topWin = getActiveControllerWindow();
        if (!topWin || !topWin->isVisible())
        {
            mControllerButtonsOverlay->setVisible(false);
            mInventoryTabsOverlay->setVisible(false);
            return;
        }

        // setButtons will handle setting visibility based on if any buttons are defined.
        mControllerButtonsOverlay->setButtons(topWin->getControllerButtons());
        if (getMode() == GM_Inventory)
        {
            mInventoryTabsOverlay->setVisible(true);
            mInventoryTabsOverlay->setTab(mActiveControllerWindows[GM_Inventory]);
        }
        else
            mInventoryTabsOverlay->setVisible(false);
    }

    MWRender::LocalMap* WindowManager::getLocalMapRender()
    {
        return mLocalMapRender.get();
    }
}
