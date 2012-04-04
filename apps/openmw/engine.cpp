#include "engine.hpp"
#include "components/esm/loadcell.hpp"

#include <cassert>

#include <iostream>
#include <utility>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

#include <MyGUI_WidgetManager.h>

#include <openengine/ogre/renderer.hpp>
#include <openengine/gui/manager.hpp>

#include <components/esm/records.hpp>
#include <components/esm_store/cell_store.hpp>
#include <components/bsa/bsa_archive.hpp>
#include <components/esm/esm_reader.hpp>
#include <components/files/fixedpath.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/settings/settings.hpp>

#include <components/nifbullet/bullet_nif_loader.hpp>
#include <components/nifogre/ogre_nif_loader.hpp>

#include "mwinput/inputmanager.hpp"

#include "mwgui/window_manager.hpp"

#include "mwscript/scriptmanager.hpp"
#include "mwscript/compilercontext.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwscript/extensions.hpp"
#include "mwscript/globalscripts.hpp"

#include "mwsound/soundmanager.hpp"

#include "mwworld/world.hpp"
#include "mwworld/class.hpp"
#include "mwworld/player.hpp"

#include "mwclass/classes.hpp"

#include "mwdialogue/dialoguemanager.hpp"
#include "mwdialogue/journal.hpp"

#include "mwmechanics/mechanicsmanager.hpp"


void OMW::Engine::executeLocalScripts()
{
    MWWorld::LocalScripts& localScripts = mEnvironment.mWorld->getLocalScripts();

    localScripts.startIteration();

    while (!localScripts.isFinished())
    {
        std::pair<std::string, MWWorld::Ptr> script = localScripts.getNext();

        MWScript::InterpreterContext interpreterContext (mEnvironment,
            &script.second.getRefData().getLocals(), script.second);
        mEnvironment.mScriptManager->run (script.first, interpreterContext);

        if (mEnvironment.mWorld->hasCellChanged())
            break;
    }

    localScripts.setIgnore (MWWorld::Ptr());
}

void OMW::Engine::updateFocusReport (float duration)
{

    if ((mFocusTDiff += duration)>0.25)
    {
        mFocusTDiff = 0;

        std::string name;

        std::string handle = mEnvironment.mWorld->getFacedHandle();

        if (!handle.empty())
        {
            // the faced handle is not updated immediately, so on a cell change it might
            // point to an object that doesn't exist anymore
            // therefore, we are catching the "Unknown Ogre handle" exception that occurs in this case
            try
            {
                MWWorld::Ptr ptr = mEnvironment.mWorld->getPtrViaHandle (handle);

                if (!ptr.isEmpty()){
                    name = MWWorld::Class::get (ptr).getName (ptr);

                }
            }
            catch (std::runtime_error& e)
            {}
        }

        if (name!=mFocusName)
        {
            mFocusName = name;

            if (mFocusName.empty())
                std::cout << "Unfocus" << std::endl;
            else
                std::cout << "Focus: " << name << std::endl;
        }
    }
}

void OMW::Engine::setAnimationVerbose(bool animverbose){
    if(animverbose){
        NifOgre::NIFLoader::getSingletonPtr()->setOutputAnimFiles(true);
        NifOgre::NIFLoader::getSingletonPtr()->setVerbosePath(mCfgMgr.getLogPath().string());
    }
}

bool OMW::Engine::frameRenderingQueued (const Ogre::FrameEvent& evt)
{
    try
    {
        mEnvironment.mFrameDuration = evt.timeSinceLastFrame;

        // sound
        if (mUseSound)
            mEnvironment.mSoundManager->update (evt.timeSinceLastFrame);

        // update GUI
        Ogre::RenderWindow* window = mOgre->getWindow();
        mEnvironment.mWindowManager->wmUpdateFps(window->getLastFPS(),
                                                 window->getTriangleCount(),
                                                 window->getBatchCount());

        mEnvironment.mWindowManager->onFrame(mEnvironment.mFrameDuration);

        // global scripts
        mEnvironment.mGlobalScripts->run (mEnvironment);

        bool changed = mEnvironment.mWorld->hasCellChanged();

        // local scripts
        executeLocalScripts(); // This does not handle the case where a global script causes a cell
                               // change, followed by a cell change in a local script during the same
                               // frame.

        // passing of time
        if (mEnvironment.mWindowManager->getMode()==MWGui::GM_Game)
            mEnvironment.mWorld->advanceTime (
                mEnvironment.mFrameDuration*mEnvironment.mWorld->getTimeScaleFactor()/3600);


        if (changed) // keep change flag for another frame, if cell changed happend in local script
            mEnvironment.mWorld->markCellAsUnchanged();

        // update actors
        std::vector<std::pair<std::string, Ogre::Vector3> > movement;
        mEnvironment.mMechanicsManager->update (movement);

        if (mEnvironment.mWindowManager->getMode()==MWGui::GM_Game)
            mEnvironment.mWorld->doPhysics (movement, mEnvironment.mFrameDuration);

        // update world
        mEnvironment.mWorld->update (evt.timeSinceLastFrame);

        // report focus object (for debugging)
        if (mReportFocus)
            updateFocusReport (mEnvironment.mFrameDuration);
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
  , mReportFocus (false)
  , mFocusTDiff (0)
  , mScriptContext (0)
  , mFSStrict (false)
  , mCfgMgr(configurationManager)
{
    std::srand ( std::time(NULL) );
    MWClass::registerClasses();
}

OMW::Engine::~Engine()
{
    delete mEnvironment.mWorld;
    delete mEnvironment.mSoundManager;
    delete mEnvironment.mGlobalScripts;
    delete mEnvironment.mMechanicsManager;
    delete mEnvironment.mDialogueManager;
    delete mEnvironment.mJournal;
    delete mEnvironment.mScriptManager;
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
    }
}

// add resources directory
// \note This function works recursively.

void OMW::Engine::addResourcesDirectory (const boost::filesystem::path& path)
{
    mOgre->getRoot()->addResourceLocation (path.string(), "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
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

void OMW::Engine::setReportFocus (bool report)
{
    mReportFocus = report;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    mFocusTDiff = 0;
    assert (!mEnvironment.mWorld);
    assert (!mCellName.empty());
    assert (!mMaster.empty());
    assert (!mOgre);

    mOgre = new OEngine::Render::OgreRenderer;

    //we need to ensure the path to the configuration exists before creating an
    //instance of ogre root so that Ogre doesn't raise an exception when trying to
    //access it
    const boost::filesystem::path configPath = mCfgMgr.getOgreConfigPath().parent_path();
    if ( !boost::filesystem::exists(configPath) )
    {
        boost::filesystem::create_directories(configPath);
    }

    // Create the settings manager and load default settings file
    Settings::Manager settings;
    const std::string localdefault = mCfgMgr.getLocalPath().string() + "/settings-default.cfg";
    const std::string globaldefault = mCfgMgr.getGlobalPath().string() + "/settings-default.cfg";

    // prefer local
    if (boost::filesystem::exists(localdefault))
        settings.loadDefault(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadDefault(globaldefault);

    // load user settings if they exist, otherwise just load the default settings as user settings
    const std::string settingspath = mCfgMgr.getUserPath().string() + "/settings.cfg";
    if (boost::filesystem::exists(settingspath))
        settings.loadUser(settingspath);
    else if (boost::filesystem::exists(localdefault))
        settings.loadUser(localdefault);
    else if (boost::filesystem::exists(globaldefault))
        settings.loadUser(globaldefault);

    mFpsLevel = settings.getInt("fps", "HUD");

    mOgre->configure(!boost::filesystem::is_regular_file(mCfgMgr.getOgreConfigPath()),
        mCfgMgr.getOgreConfigPath().string(),
        mCfgMgr.getLogPath().string(),
        mCfgMgr.getPluginsConfigPath().string(), false);

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.

    //addResourcesDirectory(mResDir);
   
    addResourcesDirectory(mResDir / "mygui");
    addResourcesDirectory(mResDir / "water");
    addResourcesDirectory(mResDir / "gbuffer");

    // Create the window
    mOgre->createWindow("OpenMW");

    loadBSA();

    // Create the world
    mEnvironment.mWorld = new MWWorld::World (*mOgre, mFileCollections, mMaster,
        mResDir, mNewGame, mEnvironment, mEncoding);

    // Create window manager - this manages all the MW-specific GUI windows
    MWScript::registerExtensions (mExtensions);

    mEnvironment.mWindowManager = new MWGui::WindowManager(mEnvironment,
        mExtensions, mFpsLevel, mNewGame, mOgre, mCfgMgr.getLogPath().string() + std::string("/"));

    // Create sound system
    mEnvironment.mSoundManager = new MWSound::SoundManager(mUseSound, mEnvironment);

    // Create script system
    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full,
        mEnvironment);
    mScriptContext->setExtensions (&mExtensions);

    mEnvironment.mScriptManager = new MWScript::ScriptManager (mEnvironment.mWorld->getStore(),
        mVerboseScripts, *mScriptContext);

    mEnvironment.mGlobalScripts = new MWScript::GlobalScripts (mEnvironment.mWorld->getStore(),
        *mEnvironment.mScriptManager);

    // Create game mechanics system
    mEnvironment.mMechanicsManager = new MWMechanics::MechanicsManager (mEnvironment);

    // Create dialog system
    mEnvironment.mJournal = new MWDialogue::Journal (mEnvironment);
    mEnvironment.mDialogueManager = new MWDialogue::DialogueManager (mEnvironment,mExtensions);

    // load cell
    ESM::Position pos;
    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
    pos.pos[2] = 0;

    if (const ESM::Cell *exterior = mEnvironment.mWorld->getExterior (mCellName))
    {
        mEnvironment.mWorld->indexToPosition (exterior->data.gridX, exterior->data.gridY,
            pos.pos[0], pos.pos[1], true);
        mEnvironment.mWorld->changeToExteriorCell (pos);
    }
    else
    {
        pos.pos[0] = pos.pos[1] = 0;
        mEnvironment.mWorld->changeToInteriorCell (mCellName, pos);
    }

    // Sets up the input system
    MWInput::MWInputManager input(*mOgre, mEnvironment.mWorld->getPlayer(),
                                  *mEnvironment.mWindowManager, mDebug, *this);
    mEnvironment.mInputManager = &input;

    std::cout << "\nPress Q/ESC or close window to exit.\n";

    mOgre->getRoot()->addFrameListener (this);

    // Play some good 'ol tunes
    mEnvironment.mSoundManager->playPlaylist(std::string("Explore"));

    // scripts
    if (mCompileAll)
    {
        std::pair<int, int> result = mEnvironment.mScriptManager->compileAll();

        if (result.first)
            std::cout
                << "compiled " << result.second << " of " << result.first << " scripts ("
                << 100*static_cast<double> (result.second)/result.first
                << "%)"
                << std::endl;
    }

    // Start the main rendering loop
    mOgre->start();

    // Save user settings
    settings.saveUser(settingspath);

    std::cout << "Quitting peacefully.\n";
}

void OMW::Engine::activate()
{
    if (mEnvironment.mWindowManager->getMode()!=MWGui::GM_Game)
        return;

    std::string handle = mEnvironment.mWorld->getFacedHandle();

    if (handle.empty())
        return;

    // the faced handle is not updated immediately, so on a cell change it might
    // point to an object that doesn't exist anymore
    // therefore, we are catching the "Unknown Ogre handle" exception that occurs in this case
    MWWorld::Ptr ptr;
    try
    {
        ptr = mEnvironment.mWorld->getPtrViaHandle (handle);

        if (ptr.isEmpty())
            return;
    }
    catch (std::runtime_error&)
    {
        return;
    }

    MWScript::InterpreterContext interpreterContext (mEnvironment,
        &ptr.getRefData().getLocals(), ptr);

    boost::shared_ptr<MWWorld::Action> action =
        MWWorld::Class::get (ptr).activate (ptr, mEnvironment.mWorld->getPlayer().getPlayer(),
        mEnvironment);

    interpreterContext.activate (ptr, action);

    std::string script = MWWorld::Class::get (ptr).getScript (ptr);

    if (!script.empty())
    {
        mEnvironment.mWorld->getLocalScripts().setIgnore (ptr);
        mEnvironment.mScriptManager->run (script, interpreterContext);
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
