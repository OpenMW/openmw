#include "engine.hpp"
#include "components/esm/loadcell.hpp"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <MyGUI_WidgetManager.h>

#include <components/bsa/bsa_archive.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

#include <components/nifbullet/bullet_nif_loader.hpp>
#include <components/nifogre/ogre_nif_loader.hpp>

#include "mwinput/inputmanagerimp.hpp"

#include "mwgui/windowmanagerimp.hpp"
#include "mwgui/cursorreplace.hpp"

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

        if (MWBase::Environment::get().getWorld()->hasCellChanged())
            break;
    }

    localScripts.setIgnore (MWWorld::Ptr());
}

void OMW::Engine::setAnimationVerbose(bool animverbose)
{
}

bool OMW::Engine::frameRenderingQueued (const Ogre::FrameEvent& evt)
{
    try
    {
        mEnvironment.setFrameDuration (evt.timeSinceLastFrame);

        // update input
        MWBase::Environment::get().getInputManager()->update(evt.timeSinceLastFrame, false);

        // sound
        if (mUseSound)
            MWBase::Environment::get().getSoundManager()->update (evt.timeSinceLastFrame);

        // global scripts
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().run();

        bool changed = MWBase::Environment::get().getWorld()->hasCellChanged();

        // local scripts
        executeLocalScripts(); // This does not handle the case where a global script causes a cell
                               // change, followed by a cell change in a local script during the same
                               // frame.

        // passing of time
        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWorld()->advanceTime (
                mEnvironment.getFrameDuration()*MWBase::Environment::get().getWorld()->getTimeScaleFactor()/3600);


        if (changed) // keep change flag for another frame, if cell changed happend in local script
            MWBase::Environment::get().getWorld()->markCellAsUnchanged();

        // update actors
        std::vector<std::pair<std::string, Ogre::Vector3> > movement;
        MWBase::Environment::get().getMechanicsManager()->update (movement, mEnvironment.getFrameDuration(),
            MWBase::Environment::get().getWindowManager()->isGuiMode());

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWorld()->doPhysics (movement, mEnvironment.getFrameDuration());

        // update world
        MWBase::Environment::get().getWorld()->update (evt.timeSinceLastFrame, MWBase::Environment::get().getWindowManager()->isGuiMode());

        // update GUI
        Ogre::RenderWindow* window = mOgre->getWindow();
        unsigned int tri, batch;
        MWBase::Environment::get().getWorld()->getTriangleBatchCount(tri, batch);
        MWBase::Environment::get().getWindowManager()->wmUpdateFps(window->getLastFPS(), tri, batch);

        MWBase::Environment::get().getWindowManager()->onFrame(evt.timeSinceLastFrame);
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
  , mDebug (false)
  , mVerboseScripts (false)
  , mNewGame (false)
  , mUseSound (true)
  , mCompileAll (false)
  , mScriptContext (0)
  , mFSStrict (false)
  , mScriptConsoleMode (false)
  , mCfgMgr(configurationManager)
{
    std::srand ( std::time(NULL) );
    MWClass::registerClasses();
}

OMW::Engine::~Engine()
{
    mEnvironment.cleanup();
    delete mScriptContext;
    delete mOgre;
}

// Load all BSA files in data directory.

void OMW::Engine::loadBSA()
{
    const Files::MultiDirCollection& bsa = mFileCollections.getCollection (".bsa");

    for (Files::MultiDirCollection::TIter iter(bsa.begin()); iter!=bsa.end(); ++iter)
    {
        std::cout << "Adding " << iter->second.string() << std::endl;
        Bsa::addBSA(iter->second.string());
    }

    const Files::PathContainer& dataDirs = mFileCollections.getPaths();
    std::string dataDirectory;
    for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
    {
        dataDirectory = iter->string();
        std::cout << "Data dir " << dataDirectory << std::endl;
        Bsa::addDir(dataDirectory, mFSStrict);

        // Workaround until resource listing capabilities are added to DirArchive, we need those to list available splash screens
        addResourcesDirectory (dataDirectory);
    }
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

// Set resource dir
void OMW::Engine::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = boost::filesystem::system_complete(parResDir);
}

// Set start cell name (only interiors for now)

void OMW::Engine::setCell (const std::string& cellName)
{
    mCellName = cellName;
}

// Set master file (esm)
// - If the given name does not have an extension, ".esm" is added automatically
// - Currently OpenMW only supports one master at the same time.

void OMW::Engine::addMaster (const std::string& master)
{
    assert (mMaster.empty());
    mMaster = master;

    // Append .esm if not already there
    std::string::size_type sep = mMaster.find_last_of (".");
    if (sep == std::string::npos)
    {
        mMaster += ".esm";
    }
}

void OMW::Engine::setDebugMode(bool debugMode)
{
    mDebug = debugMode;
}

void OMW::Engine::setScriptsVerbosity(bool scriptsVerbosity)
{
    mVerboseScripts = scriptsVerbosity;
}

void OMW::Engine::setNewGame(bool newGame)
{
    mNewGame = newGame;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mCellName.empty());
    assert (!mMaster.empty());
    assert (!mOgre);

    mOgre = new OEngine::Render::OgreRenderer;

    // Create the settings manager and load default settings file
    Settings::Manager settings;
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
    const std::string settingspath = mCfgMgr.getUserPath().string() + "/settings.cfg";
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);
    else if (boost::filesystem::exists(localdefault))
        settings.loadUser(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadUser(globaldefault);

    // Get the path for the keybinder xml file
    std::string keybinderUser = (mCfgMgr.getUserPath() / "input.xml").string();
    bool keybinderUserExists = boost::filesystem::exists(keybinderUser);

    mFpsLevel = settings.getInt("fps", "HUD");

    // load nif overrides
    NifOverrides::Overrides nifOverrides;
    if (boost::filesystem::exists(mCfgMgr.getLocalPath().string() + "/transparency-overrides.cfg"))
        nifOverrides.loadTransparencyOverrides(mCfgMgr.getLocalPath().string() + "/transparency-overrides.cfg");
    else if (boost::filesystem::exists(mCfgMgr.getGlobalPath().string() + "/transparency-overrides.cfg"))
        nifOverrides.loadTransparencyOverrides(mCfgMgr.getGlobalPath().string() + "/transparency-overrides.cfg");

    std::string renderSystem = settings.getString("render system", "Video");
    if (renderSystem == "")
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        renderSystem = "Direct3D9 Rendering Subsystem";
#else
        renderSystem = "OpenGL Rendering Subsystem";
#endif
    }
    mOgre->configure(
        mCfgMgr.getLogPath().string(),
        renderSystem,
        Settings::Manager::getString("opengl rtt mode", "Video"),
        false);

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.

    //addResourcesDirectory(mResDir);

    addResourcesDirectory(mCfgMgr.getCachePath ().string());

    addResourcesDirectory(mResDir / "mygui");
    addResourcesDirectory(mResDir / "water");
    addResourcesDirectory(mResDir / "gbuffer");
    addResourcesDirectory(mResDir / "shadows");
    addZipResource(mResDir / "mygui" / "Obliviontt.zip");

    // Create the window
    OEngine::Render::WindowSettings windowSettings;
    windowSettings.fullscreen = settings.getBool("fullscreen", "Video");
    windowSettings.window_x = settings.getInt("resolution x", "Video");
    windowSettings.window_y = settings.getInt("resolution y", "Video");
    windowSettings.vsync = settings.getBool("vsync", "Video");
    std::string aa = settings.getString("antialiasing", "Video");
    windowSettings.fsaa = (aa.substr(0, 4) == "MSAA") ? aa.substr(5, aa.size()-5) : "0";
    mOgre->createWindow("OpenMW", windowSettings);

    loadBSA();

    // cursor replacer (converts the cursor from the bsa so they can be used by mygui)
    MWGui::CursorReplace replacer;

    // Create the world
    mEnvironment.setWorld (new MWWorld::World (*mOgre, mFileCollections, mMaster,
        mResDir, mCfgMgr.getCachePath(), mNewGame, mEncoding, mFallbackMap));

    // Create window manager - this manages all the MW-specific GUI windows
    MWScript::registerExtensions (mExtensions);

    mEnvironment.setWindowManager (new MWGui::WindowManager(
        mExtensions, mFpsLevel, mNewGame, mOgre, mCfgMgr.getLogPath().string() + std::string("/"),
        mCfgMgr.getCachePath ().string(), mScriptConsoleMode));

    // Create sound system
    mEnvironment.setSoundManager (new MWSound::SoundManager(mUseSound));

    // Create script system
    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full);
    mScriptContext->setExtensions (&mExtensions);

    mEnvironment.setScriptManager (new MWScript::ScriptManager (MWBase::Environment::get().getWorld()->getStore(),
        mVerboseScripts, *mScriptContext));

    // Create game mechanics system
    mEnvironment.setMechanicsManager (new MWMechanics::MechanicsManager);

    // Create dialog system
    mEnvironment.setJournal (new MWDialogue::Journal);
    mEnvironment.setDialogueManager (new MWDialogue::DialogueManager (mExtensions, mVerboseScripts));

    // Sets up the input system
    mEnvironment.setInputManager (new MWInput::InputManager (*mOgre,
        MWBase::Environment::get().getWorld()->getPlayer(),
         *MWBase::Environment::get().getWindowManager(), mDebug, *this, keybinderUser, keybinderUserExists));

    // load cell
    ESM::Position pos;
    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
    pos.pos[2] = 0;

    mEnvironment.getWorld()->renderPlayer();

    if (const ESM::Cell *exterior = MWBase::Environment::get().getWorld()->getExterior (mCellName))
    {
        MWBase::Environment::get().getWorld()->indexToPosition (exterior->mData.mX, exterior->mData.mY,
            pos.pos[0], pos.pos[1], true);
        MWBase::Environment::get().getWorld()->changeToExteriorCell (pos);
    }
    else
    {
        pos.pos[0] = pos.pos[1] = 0;
        MWBase::Environment::get().getWorld()->changeToInteriorCell (mCellName, pos);
    }

    std::cout << "\nPress Q/ESC or close window to exit.\n";

    mOgre->getRoot()->addFrameListener (this);

    // Play some good 'ol tunes
    MWBase::Environment::get().getSoundManager()->playPlaylist(std::string("Explore"));

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

    if (!mStartupScript.empty())
        MWBase::Environment::get().getWindowManager()->executeInConsole (mStartupScript);

    // Start the main rendering loop
    mOgre->start();

    // Save user settings
    settings.saveUser(settingspath);

    std::cout << "Quitting peacefully.\n";
}

void OMW::Engine::activate()
{
    if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        return;

    std::string handle = MWBase::Environment::get().getWorld()->getFacedHandle();

    if (handle.empty())
        return;

    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaHandle (handle);

    if (ptr.isEmpty())
        return;

    MWScript::InterpreterContext interpreterContext (&ptr.getRefData().getLocals(), ptr);

    boost::shared_ptr<MWWorld::Action> action =
        MWWorld::Class::get (ptr).activate (ptr, MWBase::Environment::get().getWorld()->getPlayer().getPlayer());

    interpreterContext.activate (ptr, action);

    std::string script = MWWorld::Class::get (ptr).getScript (ptr);

    if (!script.empty())
    {
        MWBase::Environment::get().getWorld()->getLocalScripts().setIgnore (ptr);
        MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
    }

    if (!interpreterContext.hasActivationBeenHandled())
    {
        interpreterContext.executeActivation();
    }
}

void OMW::Engine::screenshot()
{
    // Count screenshots.
    int shotCount = 0;

    const std::string screenshotPath = mCfgMgr.getUserPath().string();

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

void OMW::Engine::setEncoding(const std::string& encoding)
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
