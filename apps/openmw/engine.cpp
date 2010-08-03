#include "engine.hpp"

#include <cassert>

#include <iostream>
#include <utility>

#include <components/misc/fileops.hpp>
#include <components/bsa/bsa_archive.hpp>

#include <openengine/gui/manager.hpp>
#include "mwgui/window_manager.hpp"

#include "mwinput/inputmanager.hpp"

#include "mwscript/scriptmanager.hpp"
#include "mwscript/compilercontext.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwscript/extensions.hpp"
#include "mwscript/globalscripts.hpp"

#include "mwsound/soundmanager.hpp"

#include "mwworld/world.hpp"
#include "mwworld/ptr.hpp"
#include "mwworld/environment.hpp"

#include "mwmechanics/mechanicsmanager.hpp"

#include <OgreRoot.h>

void OMW::Engine::executeLocalScripts()
{
    for (MWWorld::World::ScriptList::const_iterator iter (
        mEnvironment.mWorld->getLocalScripts().begin());
        iter!=mEnvironment.mWorld->getLocalScripts().end(); ++iter)
    {
        MWScript::InterpreterContext interpreterContext (mEnvironment,
            &iter->second.getRefData().getLocals(), MWWorld::Ptr (iter->second));
        mScriptManager->run (iter->first, interpreterContext);

        if (mEnvironment.mWorld->hasCellChanged())
            break;
    }
}

bool OMW::Engine::frameStarted(const Ogre::FrameEvent& evt)
{
    mEnvironment.mFrameDuration = evt.timeSinceLastFrame;

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
    mEnvironment.mMechanicsManager->update();

    if (focusFrameCounter++ == focusUpdateFrame)
    {
        std::pair<std::string, float> handle = mEnvironment.mWorld->getMWScene()->getFacedHandle();
        
        std::cout << "Object: " << handle.first << ", distance: " << handle.second << std::endl;

        focusFrameCounter = 0;
    }

    return true;
}

OMW::Engine::Engine()
  : mDebug (false)
  , mVerboseScripts (false)
  , mNewGame (false)
  , mScriptManager (0)
  , mScriptContext (0)
{
}

OMW::Engine::~Engine()
{
    delete mGuiManager;
    delete mEnvironment.mWorld;
    delete mEnvironment.mSoundManager;
    delete mEnvironment.mGlobalScripts;
    delete mEnvironment.mMechanicsManager;
    delete mScriptManager;
    delete mScriptContext;
}

// Load all BSA files in data directory.

void OMW::Engine::loadBSA()
{
    boost::filesystem::directory_iterator end;

    for (boost::filesystem::directory_iterator iter (mDataDir); iter!=end; ++iter)
    {
        if (boost::filesystem::extension (iter->path())==".bsa")
        {
            std::cout << "Adding " << iter->path().string() << std::endl;
            addBSA(iter->path().file_string());
        }
    }
}

// add resources directory
// \note This function works recursively.

void OMW::Engine::addResourcesDirectory (const boost::filesystem::path& path)
{
    mOgre.getRoot()->addResourceLocation (path.file_string(), "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
}

// Set data dir

void OMW::Engine::setDataDir (const boost::filesystem::path& dataDir)
{
    mDataDir = boost::filesystem::system_complete (dataDir);
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

void OMW::Engine::enableDebugMode()
{
    mDebug = true;
}

void OMW::Engine::enableVerboseScripts()
{
    mVerboseScripts = true;
}

void OMW::Engine::setNewGame()
{
    mNewGame = true;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mEnvironment.mWorld);
    assert (!mDataDir.empty());
    assert (!mCellName.empty());
    assert (!mMaster.empty());

    std::cout << "Data directory: " << mDataDir << "\n";

    const char* plugCfg = "plugins.cfg";

    mOgre.configure(!isFile("ogre.cfg"), plugCfg, false);

    addResourcesDirectory (mDataDir / "Meshes");
    addResourcesDirectory (mDataDir / "Textures");

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.
    addResourcesDirectory("resources/mygui/");

    // Create the window
    mOgre.createWindow("OpenMW");

    loadBSA();

    // Create the world
    mEnvironment.mWorld = new MWWorld::World (mOgre, mDataDir, mMaster, mNewGame, mEnvironment);

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre.getWindow(),
                                                 mOgre.getScene());

    // Create window manager - this manages all the MW-specific GUI windows
    MWScript::registerExtensions (mExtensions);

    mEnvironment.mWindowManager = new MWGui::WindowManager(mGuiManager->getGui(), mEnvironment,
        mExtensions, mNewGame);

    // Create sound system
    mEnvironment.mSoundManager = new MWSound::SoundManager;

    // Create script system
    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full,
        mEnvironment);
    mScriptContext->setExtensions (&mExtensions);

    mScriptManager = new MWScript::ScriptManager (mEnvironment.mWorld->getStore(), mVerboseScripts,
        *mScriptContext);

    mEnvironment.mGlobalScripts = new MWScript::GlobalScripts (mEnvironment.mWorld->getStore(),
        *mScriptManager);

    // Create game mechanics system
    mEnvironment.mMechanicsManager = new MWMechanics::MechanicsManager (
        mEnvironment.mWorld->getStore(), *mEnvironment.mWindowManager);

    // load cell
    ESM::Position pos;
    pos.pos[0] = pos.pos[1] = pos.pos[2] = 0;
    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
    mEnvironment.mWorld->changeCell (mCellName, pos);

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, mEnvironment.mWorld->getPlayerPos(),
                                  *mEnvironment.mWindowManager, mDebug);

    focusFrameCounter = 0;
    
    std::cout << "\nPress Q/ESC or close window to exit.\n";

    mOgre.getRoot()->addFrameListener (this);

    // Start the main rendering loop
    mOgre.start();

    std::cout << "Quitting peacefully.\n";
}
