#include "engine.hpp"

#include <stdexcept>
#include <iomanip>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <MyGUI_WidgetManager.h>

#include <SDL.h>

#include <components/compiler/extensions0.hpp>

#include <components/bsa/resources.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/translation/translation.hpp>
#include <components/nif/niffile.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/nifogre/ogrenifloader.hpp>

#include <components/esm/loadcell.hpp>

#include "mwinput/inputmanagerimp.hpp"

#include "mwgui/windowmanagerimp.hpp"

#include "mwscript/scriptmanagerimp.hpp"
#include "mwscript/extensions.hpp"
#include "mwscript/interpretercontext.hpp"

#include "mwsound/soundmanagerimp.hpp"

#include "mwworld/class.hpp"
#include "mwworld/player.hpp"
#include "mwworld/worldimp.hpp"

#include "mwclass/classes.hpp"

#include "mwdialogue/dialoguemanagerimp.hpp"
#include "mwdialogue/journalimp.hpp"

#include "mwmechanics/mechanicsmanagerimp.hpp"

#include "mwstate/statemanagerimp.hpp"

void OMW::Engine::executeLocalScripts()
{
    MWWorld::LocalScripts& localScripts = MWBase::Environment::get().getWorld()->getLocalScripts();

    localScripts.startIteration();

    while (!localScripts.isFinished())
    {
        std::pair<std::string, MWWorld::Ptr> script = localScripts.getNext();

        MWScript::InterpreterContext interpreterContext (
            &script.second.getRefData().getLocals(), script.second);
        MWBase::Environment::get().getScriptManager()->run (script.first, interpreterContext);
    }

    localScripts.setIgnore (MWWorld::Ptr());
}

bool OMW::Engine::frameStarted (const Ogre::FrameEvent& evt)
{
    if (MWBase::Environment::get().getStateManager()->getState()!=
        MWBase::StateManager::State_NoGame)
    {
        bool paused = MWBase::Environment::get().getWindowManager()->isGuiMode();
        MWBase::Environment::get().getWorld()->frameStarted(evt.timeSinceLastFrame, paused);
        MWBase::Environment::get().getWindowManager ()->frameStarted(evt.timeSinceLastFrame);
    }
    return true;
}

bool OMW::Engine::frameRenderingQueued (const Ogre::FrameEvent& evt)
{
    try
    {
        float frametime = std::min(evt.timeSinceLastFrame, 0.2f);

        mEnvironment.setFrameDuration (frametime);

        // update input
        MWBase::Environment::get().getInputManager()->update(frametime, false);

        // sound
        if (mUseSound)
            MWBase::Environment::get().getSoundManager()->update(frametime);

        // GUI active? Most game processing will be paused, but scripts still run.
        bool guiActive = MWBase::Environment::get().getWindowManager()->isGuiMode();

        // Main menu opened? Then scripts are also paused.
        bool paused = MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu);

        // update game state
        MWBase::Environment::get().getStateManager()->update (frametime);

        if (MWBase::Environment::get().getStateManager()->getState()==
            MWBase::StateManager::State_Running)
        {
            if (!paused)
            {
                // global scripts
                MWBase::Environment::get().getScriptManager()->getGlobalScripts().run();

                // local scripts
                executeLocalScripts();

                MWBase::Environment::get().getWorld()->markCellAsUnchanged();
            }

            if (!guiActive)
                MWBase::Environment::get().getWorld()->advanceTime(
                    frametime*MWBase::Environment::get().getWorld()->getTimeScaleFactor()/3600);
        }


        // update actors
        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            MWBase::Environment::get().getMechanicsManager()->update(frametime,
                guiActive);
        }

        if (MWBase::Environment::get().getStateManager()->getState()==
            MWBase::StateManager::State_Running)
        {
            MWWorld::Ptr player = mEnvironment.getWorld()->getPlayerPtr();
            if(!guiActive && player.getClass().getCreatureStats(player).isDead())
                MWBase::Environment::get().getStateManager()->endGame();
        }

        // update world
        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            MWBase::Environment::get().getWorld()->update(frametime, guiActive);
        }

        // update GUI
        MWBase::Environment::get().getWindowManager()->onFrame(frametime);
        if (MWBase::Environment::get().getStateManager()->getState()!=
            MWBase::StateManager::State_NoGame)
        {
            Ogre::RenderWindow* window = mOgre->getWindow();
            unsigned int tri, batch;
            MWBase::Environment::get().getWorld()->getTriangleBatchCount(tri, batch);
            MWBase::Environment::get().getWindowManager()->wmUpdateFps(window->getLastFPS(), tri, batch);

            MWBase::Environment::get().getWindowManager()->update();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in framelistener: " << e.what() << std::endl;
    }

    return true;
}

OMW::Engine::Engine(Files::ConfigurationManager& configurationManager)
  : mOgre (0)
  , mFpsLevel(0)
  , mVerboseScripts (false)
  , mSkipMenu (false)
  , mUseSound (true)
  , mCompileAll (false)
  , mWarningsMode (1)
  , mScriptContext (0)
  , mFSStrict (false)
  , mScriptConsoleMode (false)
  , mCfgMgr(configurationManager)
  , mEncoding(ToUTF8::WINDOWS_1252)
  , mEncoder(NULL)
  , mActivationDistanceOverride(-1)
  , mGrab(true)
  , mScriptBlacklistUse (true)

{
    std::srand ( std::time(NULL) );
    MWClass::registerClasses();

    Uint32 flags = SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE;
    if(SDL_WasInit(flags) == 0)
    {
        //kindly ask SDL not to trash our OGL context
        //might this be related to http://bugzilla.libsdl.org/show_bug.cgi?id=748 ?
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
        SDL_SetMainReady();
        if(SDL_Init(flags) != 0)
        {
            throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
        }
    }
}

OMW::Engine::~Engine()
{
    mEnvironment.cleanup();
    delete mScriptContext;
    delete mOgre;
    SDL_Quit();
}

// add resources directory
// \note This function works recursively.

void OMW::Engine::addResourcesDirectory (const boost::filesystem::path& path)
{
    mOgre->getRoot()->addResourceLocation (path.string(), "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
}

void OMW::Engine::addZipResource (const boost::filesystem::path& path)
{
    mOgre->getRoot()->addResourceLocation (path.string(), "Zip",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, false);
}

void OMW::Engine::enableFSStrict(bool fsStrict)
{
    mFSStrict = fsStrict;
}

// Set data dir

void OMW::Engine::setDataDirs (const Files::PathContainer& dataDirs)
{
    mDataDirs = dataDirs;
    mFileCollections = Files::Collections (dataDirs, !mFSStrict);
}

// Add BSA archive
void OMW::Engine::addArchive (const std::string& archive) {
    mArchives.push_back(archive);
}

// Set resource dir
void OMW::Engine::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = parResDir;
}

// Set start cell name (only interiors for now)

void OMW::Engine::setCell (const std::string& cellName)
{
    mCellName = cellName;
}

void OMW::Engine::addContentFile(const std::string& file)
{
    mContentFiles.push_back(file);
}

void OMW::Engine::setScriptsVerbosity(bool scriptsVerbosity)
{
    mVerboseScripts = scriptsVerbosity;
}

void OMW::Engine::setSkipMenu (bool skipMenu)
{
    mSkipMenu = skipMenu;
}

std::string OMW::Engine::loadSettings (Settings::Manager & settings)
{
    // Create the settings manager and load default settings file
    const std::string localdefault = mCfgMgr.getLocalPath().string() + "/settings-default.cfg";
    const std::string globaldefault = mCfgMgr.getGlobalPath().string() + "/settings-default.cfg";

    // prefer local
    if (boost::filesystem::exists(localdefault))
        settings.loadDefault(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadDefault(globaldefault);
    else
        throw std::runtime_error ("No default settings file found! Make sure the file \"settings-default.cfg\" was properly installed.");

    // load user settings if they exist, otherwise just load the default settings as user settings
    const std::string settingspath = mCfgMgr.getUserConfigPath().string() + "/settings.cfg";
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);
    else if (boost::filesystem::exists(localdefault))
        settings.loadUser(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadUser(globaldefault);

    mFpsLevel = settings.getInt("fps", "HUD");

    // load nif overrides
    NifOverrides::Overrides nifOverrides;
    std::string transparencyOverrides = "/transparency-overrides.cfg";
    std::string materialOverrides = "/material-overrides.cfg";
    if (boost::filesystem::exists(mCfgMgr.getLocalPath().string() + transparencyOverrides))
        nifOverrides.loadTransparencyOverrides(mCfgMgr.getLocalPath().string() + transparencyOverrides);
    else if (boost::filesystem::exists(mCfgMgr.getGlobalPath().string() + transparencyOverrides))
        nifOverrides.loadTransparencyOverrides(mCfgMgr.getGlobalPath().string() + transparencyOverrides);
    if (boost::filesystem::exists(mCfgMgr.getLocalPath().string() + materialOverrides))
        nifOverrides.loadMaterialOverrides(mCfgMgr.getLocalPath().string() + materialOverrides);
    else if (boost::filesystem::exists(mCfgMgr.getGlobalPath().string() + materialOverrides))
        nifOverrides.loadMaterialOverrides(mCfgMgr.getGlobalPath().string() + materialOverrides);

    return settingspath;
}

void OMW::Engine::prepareEngine (Settings::Manager & settings)
{
    mEnvironment.setStateManager (
        new MWState::StateManager (mCfgMgr.getUserDataPath() / "saves", mContentFiles.at (0)));

    Nif::NIFFile::CacheLock cachelock;

    std::string renderSystem = settings.getString("render system", "Video");
    if (renderSystem == "")
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        renderSystem = "Direct3D9 Rendering Subsystem";
#else
        renderSystem = "OpenGL Rendering Subsystem";
#endif
    }

    mOgre = new OEngine::Render::OgreRenderer;

    mOgre->configure(
        mCfgMgr.getLogPath().string(),
        renderSystem,
        Settings::Manager::getString("opengl rtt mode", "Video"));

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.

    //addResourcesDirectory(mResDir);

    addResourcesDirectory(mCfgMgr.getCachePath ().string());

    addResourcesDirectory(mResDir / "mygui");
    addResourcesDirectory(mResDir / "water");
    addResourcesDirectory(mResDir / "shadows");

    OEngine::Render::WindowSettings windowSettings;
    windowSettings.fullscreen = settings.getBool("fullscreen", "Video");
    windowSettings.window_x = settings.getInt("resolution x", "Video");
    windowSettings.window_y = settings.getInt("resolution y", "Video");
    windowSettings.screen = settings.getInt("screen", "Video");
    windowSettings.vsync = settings.getBool("vsync", "Video");
    windowSettings.icon = "openmw.png";
    std::string aa = settings.getString("antialiasing", "Video");
    windowSettings.fsaa = (aa.substr(0, 4) == "MSAA") ? aa.substr(5, aa.size()-5) : "0";

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS,
                settings.getBool("minimize on focus loss", "Video") ? "1" : "0");

    mOgre->createWindow("OpenMW", windowSettings);

    Bsa::registerResources (mFileCollections, mArchives, true, mFSStrict);

    // Create input and UI first to set up a bootstrapping environment for
    // showing a loading screen and keeping the window responsive while doing so

    std::string keybinderUser = (mCfgMgr.getUserConfigPath() / "input_v1.xml").string();
    bool keybinderUserExists = boost::filesystem::exists(keybinderUser);
    MWInput::InputManager* input = new MWInput::InputManager (*mOgre, *this, keybinderUser, keybinderUserExists, mGrab);
    mEnvironment.setInputManager (input);

    MWGui::WindowManager* window = new MWGui::WindowManager(
                mExtensions, mFpsLevel, mOgre, mCfgMgr.getLogPath().string() + std::string("/"),
                mCfgMgr.getCachePath ().string(), mScriptConsoleMode, mTranslationDataStorage, mEncoding);
    mEnvironment.setWindowManager (window);

    // Create sound system
    mEnvironment.setSoundManager (new MWSound::SoundManager(mUseSound));

    if (!mSkipMenu)
    {
        std::string logo = mFallbackMap["Movies_Company_Logo"];
        if (!logo.empty())
            window->playVideo(logo, 1);
    }

    // Create the world
    mEnvironment.setWorld( new MWWorld::World (*mOgre, mFileCollections, mContentFiles,
        mResDir, mCfgMgr.getCachePath(), mEncoder, mFallbackMap,
        mActivationDistanceOverride, mCellName, mStartupScript));
    MWBase::Environment::get().getWorld()->setupPlayer();
    input->setPlayer(&mEnvironment.getWorld()->getPlayer());

    window->initUI();
    window->renderWorldMap();

    //Load translation data
    mTranslationDataStorage.setEncoder(mEncoder);
    for (size_t i = 0; i < mContentFiles.size(); i++)
      mTranslationDataStorage.loadTranslationData(mFileCollections, mContentFiles[i]);

    Compiler::registerExtensions (mExtensions);

    // Create script system
    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full);
    mScriptContext->setExtensions (&mExtensions);

    mEnvironment.setScriptManager (new MWScript::ScriptManager (MWBase::Environment::get().getWorld()->getStore(),
        mVerboseScripts, *mScriptContext, mWarningsMode,
        mScriptBlacklistUse ? mScriptBlacklist : std::vector<std::string>()));

    // Create game mechanics system
    MWMechanics::MechanicsManager* mechanics = new MWMechanics::MechanicsManager;
    mEnvironment.setMechanicsManager (mechanics);

    // Create dialog system
    mEnvironment.setJournal (new MWDialogue::Journal);
    mEnvironment.setDialogueManager (new MWDialogue::DialogueManager (mExtensions, mVerboseScripts, mTranslationDataStorage));

    mOgre->getRoot()->addFrameListener (this);

    // scripts
    if (mCompileAll)
    {
        std::pair<int, int> result = MWBase::Environment::get().getScriptManager()->compileAll();

        if (result.first)
            std::cout
                << "compiled " << result.second << " of " << result.first << " scripts ("
                << 100*static_cast<double> (result.second)/result.first
                << "%)"
                << std::endl;
    }
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mContentFiles.empty());
    assert (!mOgre);

    Settings::Manager settings;
    std::string settingspath;

    settingspath = loadSettings (settings);

    // Create encoder
    ToUTF8::Utf8Encoder encoder (mEncoding);
    mEncoder = &encoder;

    prepareEngine (settings);

    // Play some good 'ol tunes
    MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Explore"));

    // start in main menu
    if (!mSkipMenu)
    {
        MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        try
        {
            // Is there an ini setting for this filename or something?
            MWBase::Environment::get().getSoundManager()->streamMusic("Special/morrowind title.mp3");

            std::string logo = mFallbackMap["Movies_Morrowind_Logo"];
            if (!logo.empty())
                MWBase::Environment::get().getWindowManager()->playVideo(logo, true);
        }
        catch (...) {}
    }
    else
    {
        MWBase::Environment::get().getStateManager()->newGame (true);
    }

    // Start the main rendering loop
    while (!mEnvironment.get().getStateManager()->hasQuitRequest())
        Ogre::Root::getSingleton().renderOneFrame();

    // Save user settings
    settings.saveUser(settingspath);

    std::cout << "Quitting peacefully." << std::endl;
}

void OMW::Engine::activate()
{
    if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        return;

    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getFacedObject();

    if (ptr.isEmpty())
        return;

    if (ptr.getClass().getName(ptr) == "") // objects without name presented to user can never be activated
        return;

    MWBase::Environment::get().getWorld()->activate(ptr, MWBase::Environment::get().getWorld()->getPlayerPtr());
}

void OMW::Engine::screenshot()
{
    // Count screenshots.
    int shotCount = 0;

    const std::string& screenshotPath = mCfgMgr.getUserDataPath().string();

    // Find the first unused filename with a do-while
    std::ostringstream stream;
    do
    {
        // Reset the stream
        stream.str("");
        stream.clear();

        stream << screenshotPath << "screenshot" << std::setw(3) << std::setfill('0') << shotCount++ << ".png";

    } while (boost::filesystem::exists(stream.str()));

    mOgre->screenshot(stream.str());
}

void OMW::Engine::setCompileAll (bool all)
{
    mCompileAll = all;
}

void OMW::Engine::setSoundUsage(bool soundUsage)
{
    mUseSound = soundUsage;
}

void OMW::Engine::showFPS(int level)
{
    mFpsLevel = level;
}

void OMW::Engine::setEncoding(const ToUTF8::FromType& encoding)
{
    mEncoding = encoding;
}

void OMW::Engine::setFallbackValues(std::map<std::string,std::string> fallbackMap)
{
    mFallbackMap = fallbackMap;
}

void OMW::Engine::setScriptConsoleMode (bool enabled)
{
    mScriptConsoleMode = enabled;
}

void OMW::Engine::setStartupScript (const std::string& path)
{
    mStartupScript = path;
}

void OMW::Engine::setActivationDistanceOverride (int distance)
{
    mActivationDistanceOverride = distance;
}

void OMW::Engine::setWarningsMode (int mode)
{
    mWarningsMode = mode;
}

void OMW::Engine::setScriptBlacklist (const std::vector<std::string>& list)
{
    mScriptBlacklist = list;
}

void OMW::Engine::setScriptBlacklistUse (bool use)
{
    mScriptBlacklistUse = use;
}